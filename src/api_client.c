/*
 * api_client.c
 *
 *  Created on: Mar 25, 2016
 *      Author: santiago
 */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif

#include "api_client.h"

//Private methods definitions to avoid the implicit call
REQUEST_STATE client_sendChunkSize(client_t *mClient, int chunkSize);
REQUEST_STATE client_sendFileToPull(client_t *mClient);
REQUEST_STATE client_parseFile(client_t *mClient, size_t chunkSize);
void client_outputChunkAtIndex(client_t *mClient,
		unsigned long index, unsigned int kSize);
REQUEST_STATE client_sendChunk(client_t *mClient,
		char *buffer, size_t bufferSize);
REQUEST_STATE client_pullFile(client_t *mClient, unsigned int chunkSize);

/**
 * Public
 * Initializer for a client
 * @note In case the provided files dont exist or have access rights
 * warnings will be prompted
 */
void client_init(client_t *mClient, socket_t *mSocket,
		char *localFile, char *outputFile, char *externalFile) {
	//Initialize childs if they are custom structs
	socket_init(mSocket);

	mClient->mSocket = mSocket;
	mClient->localFile = fopen(localFile, FILE_MODE_READ);

	//If the file couldnt be opened, warn it
	if (mClient->localFile == 0)
		printf("WARNING: client local file failed to open\n");

	mClient->outputFile = fopen(outputFile, FILE_MODE_WRITE);

	//If the file couldnt be opened, warn it
	if (mClient->localFile == 0)
		printf("WARNING: client output file failed to open\n");

	mClient->remoteFileName = externalFile;
}

/**
 * Public
 * Runs a client "instance"
 *
 * @param state will hold the state of the "app flow"
 *
 * @note in case an error occurs, error will be prompted and
 * no further methods will be executed. It will cascade end.
 *
 * @note Applies for the whole api. Codes are sent and received
 * using #TRANSACTION_CODE_BYTE_LENGTH to keep server and client
 * synced
 */
void client_start(client_t *mClient, char *hostName,
		char *port, int *chunkSize, APP_STATE *state) {
	*state = APP_STATE_SUCCESS; //Init our app state

	//Connect the socket to the server
	//Via the given params
	socket_connect(mClient->mSocket, hostName, port);

	/**
	 * Sidenote:
	 *
	 * Since im not too familiar with C, I dont know which
	 * are the good and bad smells.
	 *
	 * I read about the setjmp method to "fake" exceptions. But it sounded
	 * really fishy, so I will be validating everything in this
	 * really really really ugly but useful and secure way.
	 *
	 * Good point is I wont document it that much since the errors
	 * pretty much explain the flow of events
	 *
	 */

	//In case there was a connection error
	if (mClient->mSocket->connectivityState == CONNECTIVITY_ERROR) {
		printf("Error connecting to the socket\n");
		*state = APP_STATE_ERROR;
	}

	//If app is ok, send the file to the server
	if (*state == APP_STATE_SUCCESS &&
			client_sendFileToPull(mClient) == REQUEST_ERROR) {
		printf("Error trying to send remote file: %s \n", strerror(errno));
		*state = APP_STATE_ERROR;
	}

	//If app is ok, send the chunk size we will use
	if (*state == APP_STATE_SUCCESS &&
			client_sendChunkSize(mClient, *chunkSize) == REQUEST_ERROR) {
		printf("Error trying to send chunkSize: %s \n", strerror(errno));
		*state = APP_STATE_ERROR;
	}

	//If app is ok parse file and send chunk list
	if (*state == APP_STATE_SUCCESS &&
			client_parseFile(mClient, *chunkSize) == REQUEST_ERROR) {
		printf("Error trying to process chunks: %s \n", strerror(errno));
		*state = APP_STATE_ERROR;
	}

	//If app is ok wait and receive updated file and write the output
	if (*state == APP_STATE_SUCCESS &&
			client_pullFile(mClient, *chunkSize) == REQUEST_ERROR) {
		printf("Error trying to pull file: %s \n", strerror(errno));
		*state = APP_STATE_ERROR;
	}
}

/**
 * Private
 * Sends the server the file name
 *
 * @note: file name length is send in big endian
 * @note: file name length is coerced to
 * TRANSACTION_VALUE_BYTE_LENGTH bytes
 *
 * This is to keep the server and client synced
 *
 * @return state of the operation
 */
REQUEST_STATE client_sendFileToPull(client_t *mClient) {
	//Keep track of the method flow
	REQUEST_STATE state;

	//Transform the file name length to big endian
	uint32_t lengthInBigEndian = htonl(strlen(mClient->remoteFileName));

	//Coerce it to TRANSACTION_VALUE_BYTE_LENGTH bytes
	char lengthStr[TRANSACTION_VALUE_BYTE_LENGTH];
	memcpy(lengthStr, &lengthInBigEndian, TRANSACTION_VALUE_BYTE_LENGTH);

	//Send its length
	state = socket_send(mClient->mSocket,
			&lengthStr[0], TRANSACTION_VALUE_BYTE_LENGTH);

	//Check for errors
	if (state == REQUEST_ERROR) {
		printf("Error sending fileLength\n");
	} else {
		//Send the file name
		state = socket_send(mClient->mSocket,
				mClient->remoteFileName, strlen(mClient->remoteFileName));

		//Check for errors
		if (state == REQUEST_ERROR)
			printf("Error sending fileName\n");
	}

	return state;
}

/**
 * Private
 * Send to the server the chunk size we will be using
 *
 * @note: Chunk size will be sent using bigendian.
 * @note: Chunk size will be coerced to #TRANSACTION_VALUE_BYTE_LENGTH
 *
 * @return state of the operation
 */
REQUEST_STATE client_sendChunkSize(client_t *mClient, int chunkSize) {
	//Transform chunkSize to bigEndian
	uint32_t valueInBigEndian = htonl(chunkSize);

	//Coerce it to #TRANSACTION_VALUE_BYTE_LENGTH
	char charValueInBigEndian[TRANSACTION_VALUE_BYTE_LENGTH];
	memcpy(&charValueInBigEndian, &valueInBigEndian,
			TRANSACTION_VALUE_BYTE_LENGTH);

	//Send the value via socket
	REQUEST_STATE state =  socket_send(mClient->mSocket,
			(char*) &charValueInBigEndian, TRANSACTION_VALUE_BYTE_LENGTH);

	//On error
	if (state == REQUEST_ERROR)
		printf("Error sending chunk value\n");

	return state;
}

/**
 * Private
 * Parses the local file in n chunkSets and sends them
 * to the server.
 *
 * @note Chunks are sent to the server in file
 * aparition order. Since we will use their position
 * for receiving them later if appearing
 *
 * @note: Codes are sent with #TRANSACTION_CODE_BYTE_LENGTH size
 *
 * @return state of the operation
 */
REQUEST_STATE client_parseFile(client_t *mClient, size_t kSize) {
	//kSize is chunkSize. Lint things
	size_t readSize;
	char buf[kSize];

	//State for checking the flow of method
	REQUEST_STATE state = REQUEST_RECEIVING_DATA;

	//Check if localFile is accessible first
	if (mClient->localFile) {
		//Set file to the beggining to avoid problems
		rewind(mClient->localFile);

		//While we are still reading and there was no errors...
	    while (state != REQUEST_ERROR &&
	    		(readSize = fread(buf, sizeof(char), sizeof buf,
						mClient->localFile)) > 0) {
	    	/**
	    	 * If we could read a whole chunk send it
	    	 * This is because we have no guarantees
	    	 * that we will fill the window.
	    	 * Eg. File = aabbc ; chunkSize = 2
	    	 * 1st read : aa
	    	 * 2nd read : bb
	    	 * 3rd read : c?
	    	 * We cant send something with garbage.
	    	 */
	        if (readSize == kSize)
	        	state = client_sendChunk(mClient, &buf[0], kSize);
	    }

	    //If we got an error it means a chunk wasnt send
	    if (state == REQUEST_ERROR) {
	    	printf("Error sending a chunk\n");
		} else {
			//Maybe reading the file got an error ??
			if (ferror(mClient->localFile)) {
				printf("Error reading local file\n");
				state = REQUEST_ERROR;
			}
	    }
	} else { //File wasnt opened correctly. Error it
		printf("Error parsing file. Is local file opened?\n");
	}

	//If everything was ok send the 'eof parsing'
	if (state != REQUEST_ERROR) {
		char eof = TRANSACTION_CODE_EOF_CHECKSUM;
		state = socket_send(mClient->mSocket,
				&eof, TRANSACTION_CODE_BYTE_LENGTH);

		//Check for errors when sending the eof
		if (state == REQUEST_ERROR)
			printf("Error sending EOF code of checksums\n");
	}

	return state;
}

/**
 * Private
 * Sends to the server a single chunk
 *
 * @note: Codes are sent with #TRANSACTION_CODE_BYTE_LENGTH size
 * @note: Checksum value is sent with
 * #TRANSACTION_VALUE_BYTE_LENGTH size
 *
 * @note: Checksum uses Big Endian format
 *
 * Both are to keep the server and client data synced
 *
 * @return state of the operation
 */
REQUEST_STATE client_sendChunk(client_t *mClient,
		char *buffer, size_t bufferSize) {
	//Calculate the checksum for the given buffer
	checksum_t mCheckSum;
	checksum_calculate(&mCheckSum, buffer, bufferSize);

	//Convert it to big endian
	uint32_t checkSumInBigEndian = htonl(mCheckSum.checksum);

	//Code we will send
	char codeInOneByte = TRANSACTION_CODE_CHECKSUM;

	//Coerce the checksum to #TRANSACTION_VALUE_BYTE_LENGTH
	char checksumInXBytes[TRANSACTION_VALUE_BYTE_LENGTH];
	memcpy(&checksumInXBytes,
			&checkSumInBigEndian,
			TRANSACTION_VALUE_BYTE_LENGTH);

	//First send the code in TRANSACTION_CODE_BYTE_LENGTH bytes
	REQUEST_STATE state = socket_send(mClient->mSocket, &codeInOneByte,
			TRANSACTION_CODE_BYTE_LENGTH);

	//Check for errors
	if (state == REQUEST_ERROR) {
		printf("Error sending checksum code\n");
	} else {
		//Now send the checksum in its coerced size
		socket_send(mClient->mSocket,
			&checksumInXBytes[0], TRANSACTION_VALUE_BYTE_LENGTH);

		//Check again for errors
		if (state == REQUEST_ERROR)
			printf("Error sending checksum\n");
	}

	return state;
}

/**
 * Private
 * Method in charge of receiving the whole remote updated file
 * and write the output file
 *
 * @note: This method invokes #client_outputChunkAtIndex
 * @note: Codes are sent with #TRANSACTION_CODE_BYTE_LENGTH size
 * @note: Chunks are received and coerced to
 * #TRANSACTION_VALUE_BYTE_LENGTH
 *
 * @return state of the operation
 */
REQUEST_STATE client_pullFile(client_t *mClient, unsigned int chunkSize) {
	//State for controlling the method flow
	REQUEST_STATE state = REQUEST_RECEIVING_DATA;

	//Code for controlling the reception flow
	char code = TRANSACTION_CODE_UNDEFINED;

	//Check output file is accessible
	if (mClient->outputFile == 0) {
		printf("Error pulling file from server, ");
		printf("is output file opened?\n");

		state = REQUEST_ERROR;
	}

	//Only loop if state and code are OK.
    while (state == REQUEST_RECEIVING_DATA &&
    		code != TRANSACTION_CODE_EOF_PULL) {
    	//Receive from server the code for the given operation
    	state = socket_receive(mClient->mSocket,
    			(char*) &code, TRANSACTION_CODE_BYTE_LENGTH);

    	//On error occured stop here
    	if (state == REQUEST_ERROR) {
    		printf("Error receiving transaction code\n");
    	} else {
    		//Kind of switch. Maybe we could refactor it ?
    		if (code == TRANSACTION_CODE_UNIDENTIFIED_SEQUENCE_LENGTH) {
    			/**
    			 * If we got here it means theres a window
    			 * of bytes the local file doesnt have.
    			 * Receive them and write them on the output
    			 */
				//Lint forces me to use v instead of value D:
    			//This is the length of the difference
				unsigned long kValue = 0;

				//Recive the length in TRANSACTION_VALUE_BYTE_LENGTH size
				state = socket_receive(mClient->mSocket,
						(char*) &kValue, TRANSACTION_VALUE_BYTE_LENGTH);

				//Check for errors in socket op
				if (state == REQUEST_ERROR) {
					printf("Error receiving length of difference\n");
				} else {
					//Length should be in big endian. Transform to host endianess
					kValue = ntohl(kValue);

					//Create a buffer for this difference and get the str
					char buffer[kValue];
					state = socket_receive(mClient->mSocket, &buffer[0], kValue);

					//Check for op errors
					if (state == REQUEST_ERROR) {
						printf("Error receiving difference array\n");
					} else {
						//Print what asked for TP and write output
						printf("RECV File chunk %lu bytes\n", kValue);

						fwrite(&buffer[0], sizeof(char), kValue, mClient->outputFile);
					}
				}
			}

			if (code == TRANSACTON_CODE_IDENTIFIED_CHUNK) {
				/**
				 * If we got here it means the server found
				 * a same chunk from the local file.
				 * Look for it in our local file and write it
				 */
				//Value for the chunk position
				unsigned long value = 0;

				//Value is coerced to TRANSACTION_VALUE_BYTE_LENGTH !!
				state = socket_receive(mClient->mSocket,
						(char*) &value, TRANSACTION_VALUE_BYTE_LENGTH);

				//On error..
				if (state == REQUEST_ERROR) {
					printf("Error receiving position of same chunk\n");
				} else {
					//Transform position to the host endianess
					value = ntohl(value);

					//Write it in the output
					client_outputChunkAtIndex(mClient, value, chunkSize);
				}
			}
    	}
    }

	/**
	 * If EOF print if
	 * We could print it always, but since we could just
	 * get a state error and get here. Better check it
	 */
	if (code == TRANSACTION_CODE_EOF_PULL)
		printf("RECV End of file\n");

	return state;
}

/**
 * Private
 * Write in the output file the local file chunk at index #index
 * using #kSize as chunkSize
 */
void client_outputChunkAtIndex(client_t *mClient,
		unsigned long index, unsigned int kSize) {
	//Check if the localFile exists
	if (mClient->localFile) {
		char buf[kSize];

		//Clear the junk
		memset(&buf[0], '\0', kSize);

		//Move file to the chunk position
		fseek(mClient->localFile, index * kSize, SEEK_SET);

		//Store chunk in the buffer and write it
		if (fread(buf, sizeof(char), kSize, mClient->localFile) > 0) {
			printf("RECV Block index %lu\n", index);
			fwrite(&buf[0], sizeof(char), kSize, mClient->outputFile);
		}
	} else { //If file doesnt exist
		printf("Error writing a chunk, is local file opened?\n");
	}
}

/**
 * Public
 * Destructor
 */
void client_destroy(client_t *mClient) {
	if (mClient->localFile != NULL)
		fclose(mClient->localFile);

	if (mClient->outputFile != NULL)
		fclose(mClient->outputFile);

	mClient->remoteFileName = "";

	socket_destroy(mClient->mSocket);
}
