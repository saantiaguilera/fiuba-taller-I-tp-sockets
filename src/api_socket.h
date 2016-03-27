/*
 * api_socket.h
 *
 *  Created on: Mar 19, 2016
 *      Author: santiago
 */

#ifndef API_SOCKET_H_
#define API_SOCKET_H_

//Libs used
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

//Definitions for knowing what the possible filter values are
#define IPv4 AF_INET
#define IPv6 AF_INET6
#define TCP SOCK_STREAM
#define UDP SOCK_DGRAM
#define LOCALHOST AI_PASSIVE

//Definition of the local host ip
#define LOCALHOST_IP_ADDRESS "127.0.0.1"

typedef enum REQUEST_STATE {
	REQUEST_RECEIVING_DATA = 1,
	REQUEST_SOCKET_CLOSED = 0,
	REQUEST_ERROR = -1
} REQUEST_STATE;
//Enum for request states
//(aka know the return state of a send or receive)

typedef enum CONNECTIVITY_STATE {
	CONNECTIVITY_OK = 0,
	CONNECTIVITY_ERROR = -1,
	CONNECTIVITY_UNDEFINED = -2
} CONNECTIVITY_STATE;
//Enum for connection state
//(aka know the return state of a connect / listen or bind)

typedef struct socket_t {
	int socket;
	CONNECTIVITY_STATE connectivityState;
} socket_t; //Struct of a socket

typedef struct addrinfo addrinfo_t;
//For not having to type always struct addrinfo :)

//Public methods. See notes in the definition of each
//Lint complaints about 80char length. Cant comment good :(
void socket_init(socket_t *mSocket);
void socket_destroy(socket_t *mSocket);
void socket_connect(socket_t *mSocket, char *hostName, char *port);
void socket_listen(socket_t *mSocket, socket_t *newClient, int listeners);
void socket_bind(socket_t *mSocket, char *hostName, char *port);
REQUEST_STATE socket_send(socket_t *mSocket,
		char *messageData, size_t buffLength);
REQUEST_STATE socket_receive(socket_t *mSocket,
		char *response, size_t buffLength);
void socket_shutdown(socket_t *mSocket);
void socket_close(socket_t *mSocket);

#endif /* API_SOCKET_H_ */
