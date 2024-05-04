/**
 * \file workers.h
 * \brief Header file for worker functions and structures.
 *
 * This header file declares structures and functions related to processing data chunks
 * in parallel computing environments. It includes a structure for storing results of chunk processing
 * and functions for processing chunks, checking if a character is a consonant, and checking if a word
 * has two equal consonants. These utilities are essential for text analysis tasks in parallel processing applications.
 *
 * \author Pedro Rasinhas - 103541
 * \author Guilherme Antunes - 103600
 */

#ifndef WORKER_H
#define WORKER_H

#include "dispatcher.h"

/**
 * \struct ChunkResults
 * \brief Structure for storing results of chunk processing.
 *
 * This structure holds the index of the file being processed, the total number of words processed,
 * and the number of words with at least two instances of the same consonant.
 */
typedef struct ChunkResults {

    int fileIdx;
    int wordsCount;              /**< Number of words processed by the thread for this chunk */
    int wordsWithConsonants;     /**< Number of words with at least two instances of the same consonant */

} ChunkResults;


/**
 * \brief Processes a chunk of data.
 *
 * Processes a given chunk of data, counting the total number of words and the number of words
 * with at least two equal consonants. This function is designed for parallel processing,
 * allowing for efficient text analysis on large datasets.
 *
 * \param data A pointer to a Chunk structure containing the data to process.
 * \return A ChunkResults structure with the results of the processing.
 */
ChunkResults processChunk(Chunk *data);

/**
 * \brief Checks if a character is a consonant.
 *
 * Determines if a given character is a consonant, excluding vowels and the letter 'u'.
 *
 * \param c The Unicode codepoint of the character to check.
 * \return true if the character is a consonant, false otherwise.
 */
bool is_consonant(int c);

/**
 * \brief Checks if a word has two equal consonants.
 *
 * Iterates through the characters of a word to determine if it contains at least
 * two equal consonants. This function is useful for text analysis tasks.
 *
 * \param word An array of Unicode codepoints representing the word.
 * \return true if the word has two equal consonants, false otherwise.
 */
bool has_two_equal_consonants(int *word);

#endif