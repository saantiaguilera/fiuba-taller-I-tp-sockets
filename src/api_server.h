/*
 * api_server.h
 *
 *  Created on: Mar 25, 2016
 *      Author: santiago
 */

#ifndef API_SERVER_H_
#define API_SERVER_H_

#include <arpa/inet.h>
#include "api_socket.h"
#include "stdlib.h"
#include "stdio.h"
#include "stddef.h"
#include "api_checksum.h"
#include "defined_constants.h"

//Server max listeners
#define MAX_LISTENING_DEVICES 1

/**
 * Struct for a server:
 * @param mSocket is the socket from which connections will be handled
 * @param localFile is the updated remote file the client is asking to pull
 * @param mList is a list it will store in memmory of the client checksums
 */
typedef struct server_t {
	socket_t *mSocket;
	FILE *localFile;
	list_t mList;
} server_t;

//Public methods
void server_init(server_t *mServer, socket_t *mSocket);
void server_destroy(server_t *mServer);
void server_start(server_t *mServer, char *port,
		APP_STATE *state);

#endif /* API_SERVER_H_ */
