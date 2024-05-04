

#ifndef UTILS_H
#define UTILS_H
#include <mpi.h>
#include "dispatcher.h"
#include "workers.h"

long getFileSize(const char *fileName);

bool isIn(unsigned int currentChar, char *consonants, int size);

#endif