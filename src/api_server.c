/*
 * api_server.c
 *
 *  Created on: Mar 25, 2016
 *      Author: santiago
 */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif

#define _GNU_SOURCE

#include "api_server.h"

//Private methods definitions to avoid implicit calls
REQUEST_STATE server_receiveChunkSize(server_t *mServer,
		unsigned int *chunkSize);
REQUEST_STATE server_receiveFile(server_t *mServer);
REQUEST_STATE server_receiveChunks(server_t *mServer);
REQUEST_STATE server_sendRemoteFile(server_t *mServer, size_t chunkSize);
bool server_hostContainsChecksum(server_t *mServer,
		checksum_t *hostChecksum, int *position);
REQUEST_STATE server_sendChecksumInPosition(server_t *mServer, int position);
REQUEST_STATE server_sendDifferences(server_t *mServer,
		list_t *differencesList);

/**
 * Public
 * Initializer for the server
 */
void server_init(server_t *mServer, socket_t *mSocket) {
	socket_init(mSocket);

	mServer->mSocket = mSocket;
	mServer->localFile = NULL;
	list_init(&mServer->mList);
}

/**
 * Public
 * Runs a server 'instance'
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
void server_start(server_t *mServer, char *port,
		APP_STATE *state) {
	//Init socket and state for flow events
	*state = APP_STATE_SUCCESS;
	socket_t newSocket;
	socket_init(&newSocket);

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

	//Bind the socket to the given params
	socket_bind(mServer->mSocket, NULL, port);

	//If there was an error abort
	if (mServer->mSocket->connectivityState == CONNECTIVITY_ERROR) {
		printf("Error binding server socket\n");
		*state = APP_STATE_ERROR;
	}

	//If everything is OK start listening
	if (*state == APP_STATE_SUCCESS) {
		//Server will freeze here until a peer appears
		socket_listen(mServer->mSocket, &newSocket, MAX_LISTENING_DEVICES);

		//Check for errors
		if (mServer->mSocket->connectivityState == CONNECTIVITY_ERROR) {
			printf("Error listening server socket\n");
			*state = APP_STATE_ERROR;
		} else {
			//If everything was OK, close the old socket
			//And set to the server the socket that references
			//This new peer
			socket_close(mServer->mSocket);
			mServer->mSocket = &newSocket;
		}
	}

	//Alloc size for the chunkSize we will be using
	unsigned int *chunkSize = malloc(sizeof(unsigned int));

	//If app is ok, receive the remote file they want
	if (*state == APP_STATE_SUCCESS &&
			server_receiveFile(mServer) == REQUEST_ERROR) {
		printf("Error receiving remote file to pull: %s \n", strerror(errno));
		*state = APP_STATE_ERROR;
	}

	//If app is ok, receive the chunk size we will use
	if (*state == APP_STATE_SUCCESS &&
			server_receiveChunkSize(mServer,
					chunkSize) == REQUEST_ERROR) {
		printf("Error trying to receive chunkSize: %s \n", strerror(errno));
		*state = APP_STATE_ERROR;
	}

	//If app is ok, receive the chunks from their local file
	if (*state == APP_STATE_SUCCESS &&
			server_receiveChunks(mServer) == REQUEST_ERROR) {
		printf("Error receiveing the chunks: %s \n", strerror(errno));
		*state = APP_STATE_ERROR;
	}

	//If app is ok, send them the remote file chunked
	if (*state == APP_STATE_SUCCESS &&
			server_sendRemoteFile(mServer,
					*chunkSize) == REQUEST_ERROR) {
		printf("Error trying to send file client: %s \n", strerror(errno));
		*state = APP_STATE_ERROR;
	}

	//Free chunkSize allocation
	free(chunkSize);
}

/**
 * Private
 * Receives the file the client wants to pull
 *
 * @note File name length is received in big endian
 * @note File name length is coerced to
 * #TRANSACTION_VALUE_BYTE_LENGTH
 *
 * @return state of the operation
 */
REQUEST_STATE server_receiveFile(server_t *mServer) {
	//Init vars. State will handle flow of the method
	uint32_t fileNameLength;
	REQUEST_STATE state = REQUEST_RECEIVING_DATA;

	//Receive the length of the file name
	state = socket_receive(mServer->mSocket,
			(char*) &fileNameLength, TRANSACTION_VALUE_BYTE_LENGTH);


	//Check for error and abort if found
	if (state == REQUEST_ERROR) {
		printf("Error receiving fileName Length\n");
		return state;
	}

	//Convert to host endian
	fileNameLength = ntohl(fileNameLength);

	//Alloc memmory for the filename string
	//Is this necessary ? @Refactor check if cant just
	//use stack
	char *fileName = malloc(fileNameLength + 1);

	//Receive the file name
	state = socket_receive(mServer->mSocket, fileName, fileNameLength);

	//Check for errors, if found free mem and abort
	if (state == REQUEST_ERROR) {
		printf("Error receiving fileName\n");
		//Free mem
		free(fileName);

		return state;
	}

	//Add a zero termination
	fileName[fileNameLength] = 0;

	//Create localFile with the fileName
	mServer->localFile = fopen(fileName, FILE_MODE_READ);

	//Check the file could open correctly
	if (mServer->localFile == 0) {
		printf("Error trying to open local file: %s \n", strerror(errno));
		state = REQUEST_ERROR;
	}

	//Free mem
	free(fileName);

	return state;
}

/**
 * Private
 * Receives the chunk size we will use in the whole run
 *
 * @note Chunk size received will be in big endian
 * @note Chunk size coerced to
 * #TRANSACTION_VALUE_BYTE_LENGTH bytes
 *
 * @return state of the operation
 */
REQUEST_STATE server_receiveChunkSize(server_t *mServer,
		unsigned int *chunkSize) {
	//Init values we will use
	REQUEST_STATE state;
	uint32_t value;

	//Receive the chunk size
	state = socket_receive(mServer->mSocket,
			(char*) &value,
			TRANSACTION_VALUE_BYTE_LENGTH);

	//Check for errors
	if (state == REQUEST_ERROR) {
		printf("Error receiving the chunk size\n");
	} else {
		//Convert from host endian and set
		value = ntohl(value);

		*chunkSize = value;
	}

	return state;
}

/**
 * Private
 * Method for receiving the list of chunks the
 * client has
 *
 * @note code are coerced to TRANSACTION_CODE_BYTE_LENGTH
 * @note checksums are coerced to
 * #TRANSACTION_VALUE_BYTE_LENGTH
 * @note checksums are received in big endian
 *
 * @return state of the operation
 */
REQUEST_STATE server_receiveChunks(server_t *mServer) {
	//Init values
	REQUEST_STATE state = REQUEST_RECEIVING_DATA;
	unsigned long value;
	char code = TRANSACTION_CODE_UNDEFINED;

	//Loop while client doesnt tell us to stop & no errors
	while (code != TRANSACTION_CODE_EOF_CHECKSUM &&
			state != REQUEST_ERROR) {
		//Receive the code of the request
		state = socket_receive(mServer->mSocket,
				(char*) &code, TRANSACTION_CODE_BYTE_LENGTH);

		//Check for errors
		if (state == REQUEST_ERROR) {
			printf("Error receiving code of chunk\n");
		} else if (code != TRANSACTION_CODE_EOF_CHECKSUM) {
			//As long as the code wasnt the eof of this loop
			//Receive the value of the checksum
			state = socket_receive(mServer->mSocket,
					(char*) &value, TRANSACTION_VALUE_BYTE_LENGTH);

			//Check for errors
			if (state == REQUEST_ERROR)
				printf("Error receiving a chunk\n");

			//Transform to host endianess
			value = ntohl(value);

			//Add it to the list
			list_add(&mServer->mList, value);
		}
	}

	return state;
}

/**
 * Private
 * Parses the whole remote file asked for pull
 * Analizes differences:
 *  - If chunks arent present,
 *  	saves the window and rolls 1 byte, repeating
 *  - If chunks are present, sends the window & list position
 *
 * @note Codes are coerced to TRANSACTION_CODE_BYTE_LENGTH size
 *
 * @note This method invokes #sendChecksumPosition and
 * #sendDifferences
 *
 * @param kSize is the chunkSize (Lint things)
 *
 * @return state of the operation
 */
REQUEST_STATE server_sendRemoteFile(server_t *mServer, size_t kSize) {
	//Init vars. State will controll flow of method
	REQUEST_STATE state = REQUEST_RECEIVING_DATA;
	char buf[kSize + 1]; //Buffer used as window
	checksum_t mChecksum; //Checksum for saving data
	int position = 0; //Position of a found checksum

	/**
	 * Ive tried a lot of things here. From mallocking
	 * everytime I got size overflow to using asprintf
	 * and freeing memmory til death.
	 *
	 * Both of them made Valgrind complaint a lot, so
	 * went for the slow but secure way.
	 *
	 * We will be using a list to store each char
	 * that is not present in the local client file
	 */
	list_t differencesList;
	list_init(&differencesList);

	//Check that localFile ("remote") is accessible
	if (mServer->localFile) {
		//Move cursor to begining
		rewind(mServer->localFile);

		/**
		 * Pay close atention to both.
		 * - newChar will be the read output of the localfile
		 * - oldChar will be the char getting out of the buffer
		 * when rolling a byte
		 *
		 * @note: As long as the "Buffer" is not filled
		 * old char will remain empty, thus we will use
		 * it as a checker (if its 0 right when the buffer
		 * got filled, it means we cant use rolling and
		 * must calculate it in the normal way)
		 *
		 * @note Everytime there is a checksum found,
		 * buffer will be cleared and so the oldChar
		 * So this can be repeated n times
		 */
		char newChar;
		char oldChar = 0;

		//Clear junk data
		memset(&buf[0], '\0', kSize + 1);

		//While no errors and file can give me a char
		while (state != REQUEST_ERROR &&
				(newChar = fgetc(mServer->localFile)) != EOF) {
			//WE JUST READ A CHAR
			//If the buffer still aint filled
			if (strlen(buf) != kSize) {
				//Append the read char
				buf[strlen(buf)] = newChar;
			} else {
				/**
				 * If its filled, before setting the
				 * new char to the buffer, save the
				 * older one and add it to the difference
				 * list
				 */
				oldChar = buf[0];
				list_add(&differencesList, oldChar);

				//Really ugly way of doing a rolling byte
				//Eg. abcd | e -> a | bcde
				for (int i = 0 ; i < kSize - 1 ; i++)
					buf[i] = buf[i+1];
				buf[kSize - 1] = newChar;
			}

			/**
			 * Check that our buffer is filled,
			 * if not let it loop and fill itself
			 */
			if (strlen(buf) == kSize) {
				if (oldChar == 0) {
					//If there is no oldChar
					//This is right when a buffer got filled
					//Calculate its checksum
					checksum_calculate(&mChecksum, &buf[0], strlen(buf));

					//If exists in the clients file
					if (server_hostContainsChecksum(mServer, &mChecksum, &position)) {
						/**
						 * Just send it.
						 *
						 * The difference list should be 0 at this point
						 * Since the buffer just got filled. This means:
						 *  - Its either its first time, and there wasnt any
						 * rolling bytes, or
						 *  - There was a rolling checksum that was found
						 * and it got flushed and sent to the client.
						 *
						 * Either ways size is 0 so dont mind it.
						 */
						state = server_sendChecksumInPosition(mServer, position);

						//Clear buff and the oldChar
						memset(&buf[0], '\0', kSize + 1);
						oldChar = 0;
					}
				} else {
					//Old char exists, we can perform rolling byte
					checksum_roll_ahead(&mChecksum, &buf[0], &oldChar, strlen(buf));

					//Check if checksum exists
					if (server_hostContainsChecksum(mServer, &mChecksum, &position)) {
						//Flush the differences
						state = server_sendDifferences(mServer, &differencesList);

						//If no errors send its position
						if (state != REQUEST_ERROR)
							server_sendChecksumInPosition(mServer, position);

						//Clear list, buffer and the old char
						list_destroy(&differencesList);
						list_init(&differencesList);
						memset(&buf[0], '\0', kSize + 1);
						oldChar = 0;
					}
				}
			}
		}

		//We finished reading the file. If no error found
		if (state != REQUEST_ERROR) {
			//Check for the last buffer window and add it
			//To the difference list if it has data
			if (strlen(&buf[0]) > 0) {
				for (int i = 0 ; i < strlen(&buf[0]) ; i++)
					list_add(&differencesList, buf[i]);
			}

			//If list has data, flush it
			if (list_count(&differencesList) > 0)
				state = server_sendDifferences(mServer, &differencesList);

			//Maybe reading the file got an error ??
			if (ferror(mServer->localFile)) {
				printf("Error reading local file\n");
				state = REQUEST_ERROR;
			}
		}
	}

	//Check that there were no errors
	if (state != REQUEST_ERROR) {
		//Send an eof of the whole operation
		char eof = TRANSACTION_CODE_EOF_PULL;
		state = socket_send(mServer->mSocket, &eof, TRANSACTION_CODE_BYTE_LENGTH);

		//Check for errors
		if (state == REQUEST_ERROR)
			printf("Error sending eof code\n");
	}

	//Free the list!
	list_destroy(&differencesList);

	return state;
}

/**
 * Private
 * Checks if the host file has a given checksum
 *
 * @note O(n)
 * @note Checksum list should already be
 * retreived from the client else it wont
 * work
 *
 * @returns checksum found or not
 */
bool server_hostContainsChecksum(server_t *mServer,
		checksum_t *hostChecksum, int *position) {
	//Iterate through the list and see if its the same
	for(int i = 0 ; i < list_count(&mServer->mList) ;  i++) {
		uint32_t checksum = list_get(&mServer->mList, i);

		if (checksum == hostChecksum->checksum) {
			*position = i;
			return true;
		}
	}

	return false;
}

/**
 * Private
 * Sends the position of a found checksum to the client
 *
 * @note code is coerced to TRANSACTION_CODE_BYTE_LENGTH
 * @note position is sent in big endian
 * @note position is coerced to
 * #TRANSACTION_VALUE_BYTE_LENGTH bytes
 *
 * @return state of the operation
 */
REQUEST_STATE server_sendChecksumInPosition(server_t *mServer, int position) {
	//Send the code to the client
	char code = TRANSACTON_CODE_IDENTIFIED_CHUNK;
	REQUEST_STATE state = socket_send(mServer->mSocket,
				&code, TRANSACTION_CODE_BYTE_LENGTH);

	//Check for errors and return if found
	if (state == REQUEST_ERROR) {
		printf("Error sending a checksum code\n");
		return state;
	}

	//Transform to big endian
	uint32_t positionInBigEndian = htonl(position);

	//Coerce it to #TRANSACTION_VALUE_BYTE_LENGTH
	char positionInBigEndianInXBytes[TRANSACTION_VALUE_BYTE_LENGTH];
	memcpy(&positionInBigEndianInXBytes,
			&positionInBigEndian,
			TRANSACTION_VALUE_BYTE_LENGTH);

	//Send it
	state = socket_send(mServer->mSocket, (char*) &positionInBigEndianInXBytes,
			TRANSACTION_VALUE_BYTE_LENGTH);

	//Check for errors
	if (state == REQUEST_ERROR)
		printf("Error sending a checksum position\n");

	return state;
}

/**
 * Private
 * Sends a window of different values that dont are
 * present in the local file.
 *
 * @note length is sent in big endian
 * @note length is coerced to #TRANSACTION_VALUE_BYTE_LENGTH
 *
 * @note code is coerced to #TRANSACTION_CODE_BYTE_LENGTH
 *
 * @return state of the operation
 */
REQUEST_STATE server_sendDifferences(server_t *mServer,
		list_t *differencesList) {
	//Send the operation code
	char diffCode = TRANSACTION_CODE_UNIDENTIFIED_SEQUENCE_LENGTH;
	REQUEST_STATE state = socket_send(mServer->mSocket,
			&diffCode, TRANSACTION_CODE_BYTE_LENGTH);

	//Check for errors and return if present
	if (state == REQUEST_ERROR) {
		printf("Error sending code for a difference chunk\n");
		return state;
	}

	//kLength is size of list. Lint things
	//Conver to big endian
	int kLength = list_count(differencesList);
	uint32_t differenceLengthInBigEndian = htonl(kLength);

	//Coerce to #TRANSACTION_VALUE_BYTE_LENGTH size
	char differenceLengthInBigEndianInXBytes[TRANSACTION_VALUE_BYTE_LENGTH];
	memcpy(&differenceLengthInBigEndianInXBytes,
			&differenceLengthInBigEndian,
			TRANSACTION_VALUE_BYTE_LENGTH);

	//Send the length
	socket_send(mServer->mSocket,
			&differenceLengthInBigEndianInXBytes[0],
			TRANSACTION_VALUE_BYTE_LENGTH);

	//Check for errors and return if found
	if (state == REQUEST_ERROR) {
		printf("Error sending a length of a difference\n");
		return state;
	}

	//Create a buffer of the difference list
	char differenceChars[kLength];

	for (int i = 0 ; i < kLength ; i++)
		differenceChars[i] = list_get(differencesList, i);

	//Send the buffer
	state = socket_send(mServer->mSocket, &differenceChars[0], kLength);

	//Print error if existing
	if (state == REQUEST_ERROR)
		printf("Error sending  a difference\n");

	return state;
}

/**
 * Public
 * Destructor
 */
void server_destroy(server_t *mServer) {
	if (mServer->localFile != NULL)
		fclose(mServer->localFile);

	list_destroy(&mServer->mList);
}
