/*
 ============================================================================
 Name        : tp.c
 Author      : Santiago Aguilera
 Version     :
 Copyright   : 
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif

#include <stdlib.h>
#include "api_socket.h"
#include "api_checksum.h"
#include "api_client.h"
#include "api_server.h"
#include "defined_constants.h"

/**
 * @TEST Params (Testing purpose only)
 * @ server 3590
 * @ client 127.0.0.1 3590 /home/santiago/workspace/tp1_new/Debug/ejemplo1 /home/santiago/workspace/tp1_new/Debug/outputFile /home/santiago/workspace/tp1_new/Debug/ejemplo2 4
 * TODO Document the new and changed stuff
 */
int main(int argc, char *argv[]) {
	//Initialize our app state
	APP_STATE state = APP_STATE_UNDEFINED;

	/**
	 * If we have at least the type parameter (this means client/server)
	 * then we should at least be able to operate and let the app flow handle
	 * the result.
	 * Which is pretty obvious it will fail since theres nothing to pull
	 * from the server.
	 * @Refactor maybe ?
	 */
	if (argc > 1) {
		//Initialize a socket we will be using
		socket_t mSocket;
		socket_init(&mSocket);

		//If client
		if (!strcmp(argv[1], HOST_CLIENT)) {
			client_t mClient;
			client_init(&mClient, &mSocket, argv[4], argv[5], argv[6]);

			int chunkSize = atoi(argv[7]);

			/**
			 * This is the core of the client.
			 * Here it will connect the socket with the given params,
			 * Send the server the specifications of the pull,
			 * Parse its src file in checksums, send the list and
			 * listen for the remote changes, while writing them to the
			 * ouput file.
			 */
			client_start(&mClient, argv[2], argv[3], &chunkSize, &state);

			client_destroy(&mClient);
		}

		//If server
		/**
		 * @note: I have done them like this (and not in if/else)
		 * since its not specified how the app should react to
		 * a random param name (eg ./app random). In this case, it just closes
		 * and nothing happens
		 */
		if (!strcmp(argv[1], HOST_SERVER)) {
			server_t mServer;
			server_init(&mServer, &mSocket);

			/**
			 * This is the core of the server.
			 * Here it will bind and listen to the supplied port,
			 * Receive the specifications of the pull and the checksum list
			 * Iterate through its own updated file while looking for differences
			 * sending them to the client, so contents are identical
			 */
			server_start(&mServer, argv[2], &state);

			server_destroy(&mServer);
		}

		//Close the file descriptor
		socket_close(&mSocket);
	} else {
		//Notify about usage if no args passed
		printf("Fail to receive arguments\n");

		printf("For client use: \"client\" @host @port");
		printf("@src @dst @remote @chunkSize\n");

		printf("For server use: \"server\" @port");

		state = APP_STATE_ERROR;
	}

	return state;
}
