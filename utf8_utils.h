/**
 * \file utf8_utils.h
 * \brief Header file for UTF-8 utility functions.
 *
 * This header file declares utility functions for handling UTF-8 encoded strings.
 * It includes functions for identifying separation characters, whitespace, punctuation marks,
 * aggregation marks, determining the number of bytes in a UTF-8 character, checking if a character
 * is alphanumeric, converting UTF-8 bytes to a Unicode codepoint, and converting characters to lowercase.
 * These utilities are essential for text processing tasks in parallel processing applications.
 *
 * \author Pedro Rasinhas - 103541
 * \author Guilherme Antunes - 103600
 */

#ifndef UTF8_UTILS_H
#define UTF8_UTILS_H

#include "dispatcher.h"
#include "workers.h"

bool isSeparationCharacter(unsigned int character);
bool isWhiteSpace(unsigned int character);
bool isPunctuationMark(unsigned int character);
bool isAggregationMark(unsigned int character);
bool isAlphanumeric(unsigned int character);
int numOfBytesInUTF8(unsigned char character);
int utf8_to_codepoint(unsigned char *bytes, int num_bytes);
int to_lower_case(int c);

#endif