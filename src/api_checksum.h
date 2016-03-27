/*
 * api_checksum.h
 *
 *  Created on: Mar 20, 2016
 *      Author: santiago
 */

#ifndef API_CHECKSUM_H_
#define API_CHECKSUM_H_

#include "stddef.h"
#include "string.h"
#include "stdio.h"
#include <arpa/inet.h>
#include "api_list.h"

//Value for not letting our hash value go further than 2^16 (65536)
#define CHECKSUM_THRESHOLD 65536

/**
 * Struct for the checksum
 * @param lower is an internal param used (should have like private access?)
 * @note: higher is an internal param used (should have like private access?)
 */
typedef struct checksum {
	unsigned long lower;
	unsigned long higher;
	unsigned long checksum;
} checksum_t;

//Public methods
void checksum_init(checksum_t *mChecksum, uint32_t checksum);
void checksum_destroy(checksum_t *mCheckSum);
void checksum_calculate(checksum_t *mCheckSum, char *buffer, size_t size);
void checksum_roll_ahead(checksum_t *lastCheckSum,
		char *buffer, char* lastChar, size_t size);

#endif /* API_CHECKSUM_H_ */
