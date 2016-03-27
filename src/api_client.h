/*
 * api_client.h
 *
 *  Created on: Mar 25, 2016
 *      Author: santiago
 */

#ifndef API_CLIENT_H_
#define API_CLIENT_H_

#include <arpa/inet.h>
#include "api_socket.h"
#include "stdlib.h"
#include "stdio.h"
#include "stddef.h"
#include <errno.h>
#include <string.h>
#include "api_checksum.h"
#include "defined_constants.h"

/**
 * Struct for a client
 * @param mSocket is the socke that will handle the connections
 * @param localFile is the file to pull/update
 * @param outputFile is the updated file from server
 * @param remoteFileName is the server file name
 */
typedef struct client_t {
	socket_t *mSocket;
	FILE *localFile;
	FILE *outputFile;
	char *remoteFileName;
} client_t;

//Public methods
void client_init(client_t *mClient, socket_t *mSocket,
		char *localFile, char *outputFile,
		char *externalFile);
void client_destroy(client_t *mClient);
void client_start(client_t *mClient, char *hostName,
		char *port, int *chunkSize, APP_STATE *state);

#endif /* API_CLIENT_H_ */
