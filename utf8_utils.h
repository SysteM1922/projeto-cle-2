#ifndef UTF8_UTILS_H
#define UTF8_UTILS_H

#include "dispatcher.h"
#include "workers.h"

bool isSeparationCharacter(unsigned int character);
bool isWhiteSpace(unsigned int character);
bool isPunctuationMark(unsigned int character);
bool isAggregationMark(unsigned int character);


#endif