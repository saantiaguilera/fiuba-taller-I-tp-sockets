/*
 * api_checksum.c
 *
 *  Created on: Mar 20, 2016
 *      Author: santiago
 */

#include "api_checksum.h"

/**
 * Public. Creates a checksum that only has the value
 * @note This initializer cant be used with
 * api_checksum.c#checksum_roll_ahead or
 * api_checksum.c#checksum_calculate
 *
 * @note Its just a wrapper of your checksum value
 */
void checksum_init(checksum_t *mChecksum, uint32_t checksum) {
	mChecksum->lower = 0;
	mChecksum->higher = 0;
	mChecksum->checksum = checksum;
}

/**
 * Private. Inits a new checksum
 * @note User shouldnt be able to create checksums
 * on his own (something like private constructors ?)
 * @note The "checksum value" is calculated inside
 * here, taking into account the lower and higher values
 *
 * @note Im having the lower and higher params
 * inside the entity because of the "byte roll" method
 */
void checksum_create_from_params(checksum_t *mCheckSum,
		unsigned long lower, unsigned long higher) {
	mCheckSum->lower = lower;
	mCheckSum->higher = higher;
	mCheckSum->checksum = lower + (higher * CHECKSUM_THRESHOLD);
}

/**
 * Destructor
 */
void checksum_destroy(checksum_t *mCheckSum) {
	mCheckSum->lower = -1;
	mCheckSum->higher = -1;
	mCheckSum->checksum = -1;
}

/**
 * Calculates the checksum for the
 * given buffer among the size
 *
 * @param buffer is a pointer to the
 * first char of the block to be checksumed
 * @param size is the size of the buffer
 * @param mCheckSum is a pointer to the
 * checksum were im going to deliver results
 *
 * @note Is slow so be wary.
 * @returns checksum inside mCheckSum.
 */
void checksum_calculate(checksum_t *mCheckSum, char *buffer, size_t size) {
	unsigned long lower = 0;
	unsigned long higher = 0;

	//Iterate through the given buffer (not causing
	//a buffer overflow) and apply the Adler32 formulae
	for (int i = 0 ; i < size ; i++) {
		lower += buffer[i];
		higher += ((size - i) * buffer[i]);
	}

	//Mod the values in case they went
	//further than our threshold
	lower %= CHECKSUM_THRESHOLD;
	higher %= CHECKSUM_THRESHOLD;

	checksum_create_from_params(mCheckSum, lower, higher);
}

/**
 * Calculates the checksum of 1 byte ahead the last one.
 * Faster than checksum_calculate, but moves
 * with a 1byte step
 *
 * @param buffer is a pointer to the first
 * char of the block to be checksumed
 * @param size is the size of the buffer
 * @param @NonNull mCheckSum is the previous checksum AND
 * a pointer to the checksum were im going to deliver results
 *
 * @note: You should be at least 1 byte ahead from
 * your start buffer, else this wont work as intended
 * @returns lastCheckSum will be overwritten with the
 * new checkSum data (this means you will loose your
 * "pre last checksum")
 */
void checksum_roll_ahead(checksum_t *lastCheckSum, char *buffer,
		char* lastChar, size_t size) {
	//Using the Adler32 formulae calculate our new params
	unsigned long newLower = ((lastCheckSum->lower -
			(unsigned long) *lastChar + (unsigned long) buffer[size-1])
			% CHECKSUM_THRESHOLD);

	unsigned long newHigher = ((lastCheckSum->higher -
			(size * (unsigned long) *lastChar) + newLower)
			% CHECKSUM_THRESHOLD);

	checksum_create_from_params(lastCheckSum, newLower, newHigher);
}
