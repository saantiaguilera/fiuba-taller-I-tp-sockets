/*
 * defined_constants.h
 *
 *  Created on: Mar 26, 2016
 *      Author: santiago
 */

#ifndef DEFINED_CONSTANTS_H_
#define DEFINED_CONSTANTS_H_

/**
 * Transactions byte lengths.
 * This is used mainly to keep consistency across the length of a "chunk"
 * or a "code".
 * If we want for example, to send the checksums in 8b instead of 4b just change the
 * transaction value byte.
 */
#define TRANSACTION_CODE_BYTE_LENGTH 1
#define TRANSACTION_VALUE_BYTE_LENGTH 4

/**
 * File opening modes used
 */
#define FILE_MODE_READ "r"
#define FILE_MODE_WRITE "w"

/**
 * Possible params for using this app
 */
#define HOST_CLIENT "client"
#define HOST_SERVER "server"

/**
 * @TEST purpose
 * @Private access
 */
#define DEFAULT_HOSTNAME "localhost"
#define DEFAULT_PORT "3590"

/**
 * Transactions types.
 * Also to keep consistency. If you want to change one, this will
 * broadcast the change along everyone so its easier refactoring.
 */
typedef enum TRANSACTION_CODES {
	TRANSACTION_CODE_UNDEFINED = 0,
	TRANSACTION_CODE_CHECKSUM = 1,
	TRANSACTION_CODE_EOF_CHECKSUM = 2,
	TRANSACTION_CODE_UNIDENTIFIED_SEQUENCE_LENGTH = 3,
	TRANSACTON_CODE_IDENTIFIED_CHUNK = 4,
	TRANSACTION_CODE_EOF_PULL = 5
} TRANSACTION_CODES;

/**
 * App state to know the state of
 * the aplication in runtime and handle
 * stuff accordingly
 */
typedef enum APP_STATE {
	APP_STATE_UNDEFINED = -1,
	APP_STATE_SUCCESS = 0,
	APP_STATE_ERROR = 1
} APP_STATE;

#endif /* DEFINED_CONSTANTS_H_ */
