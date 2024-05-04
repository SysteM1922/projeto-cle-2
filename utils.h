/**
 * \file utils.h
 * \brief Header file for utility functions.
 *
 * This header file declares utility functions for file handling and character checking.
 * It includes functions for determining the size of a file and checking if a character is in a given array.
 * These utilities are essential for file processing tasks and character analysis in parallel processing applications.
 *
 * \author Pedro Rasinhas - 103541
 * \author Guilherme Antunes - 103600
 */

#ifndef UTILS_H
#define UTILS_H
#include <mpi.h>
#include "dispatcher.h"
#include "workers.h"

long getFileSize(const char *fileName);

bool isIn(unsigned int currentChar, char *consonants, int size);

#endif