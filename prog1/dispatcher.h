/**
 * \file dispatcher.h
 * \brief Header file for the dispatcher module.
 *
 * This file contains the definitions of structures and function prototypes
 * used for setting up file processing, dispatching tasks, and aggregating results.
 * It includes structures for file information, file data, and chunks of data, as well as
 * functions for initializing the dispatcher, retrieving chunks of data,
 * aggregating results, and printing results.
 *
 * \author Pedro Rasinhas - 103541
 * \author Guilherme Antunes - 103600
 */

#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <stdbool.h>
#include <stdio.h>

#define DEFAULT_CHUNK_SIZE 4096 // 4KB
#define MAX_PROCESSES 8
#define MAX_FILES 20
#define MAX_FILE_PATH_SIZE 128

/**
 * \brief Structure for storing information about the current file being processed.
 *
 * Contains the current file index, a pointer to the file, the current position in the file,
 * and the size of the file.
 */
typedef struct File{
    short currentFileIdx;
    FILE* currentFILE;
    int currentPosition;     /**< Current position in the file */
    int fileSize;            /**< Size of the file */
} File;

/**
 * \brief Structure for storing information about the files to be processed.
 *
 * Contains an array to store filenames and the total number of files.
 */
typedef struct FilesInfo {
    char* filenames[MAX_FILES]; // Array to store filenames
    int nFiles;
} FilesInfo;

/**
 * \brief Structure for storing data about each file.
 *
 * Contains the file index, the number of words processed by the thread for this chunk,
 * and the number of words with at least two instances of the same consonant.
 */
typedef struct FileData {
    int fileIdx;
    int wordsCount;              /**< Number of words processed by the thread for this chunk */
    int wordsWithConsonants;     /**< Number of words with at least two instances of the same consonant */
} FileData;

/**
 * \brief Structure for storing a chunk of data read from the file.
 *
 * Contains the chunk of data, a flag indicating if it's the final chunk, the file index,
 * and the start and end positions of the chunk in the file.
 */
typedef struct Chunk {
    unsigned char* chunk;       /**< Chunk of data read from the file */
    bool isFinal;
    int fileIdx;
    int startPosition;          /**< Start position of the chunk in the file */
    int endPosition;            /**< End position of the chunk in the file */
} Chunk;

/**
 * \brief Sets up the necessary structures for file processing.
 *
 * Allocates memory for the FilesInfo structure and parses command-line arguments.
 *
 * \param argc The number of command-line arguments.
 * \param argv The array of command-line arguments.
 * \return The number of files to be processed, or -1 on error.
 */
int setupFiles(int argc, char *argv[]);

/**
 * \brief Initializes the dispatcher with the number of files and processes.
 *
 * Allocates memory for the File and FileData structures and initializes them.
 *
 * \param numFiles The number of files to be processed.
 * \param nProcesses The number of processes to be used for processing.
 * \return 0 on success, -1 on error.
 */
int setupDispatcher(int numFiles);

/**
 * \brief Retrieves the next chunk of data from the current file.
 *
 * Reads a chunk of data from the current file, handling file boundaries and UTF-8 encoding.
 *
 * \return A Chunk structure containing the chunk data, file index, and position information.
 */
Chunk getChunk();

/**
 * \brief Aggregates results from file processing.
 *
 * Updates the FileData structure with the number of words and words with consonants.
 *
 * \param fileIdx The index of the file being processed.
 * \param words The number of words found in the file.
 * \param consonants The number of words with consonants found in the file.
 */
void aggregateResults(int fileIdx, int words, int consonants);

/**
 * \brief Prints the aggregated results for each file.
 *
 * Outputs the file index, word count, and words with consonants for each file.
 */
void printResults();

/**
 * \brief Retrieves the chunk size specified by the user, or the default chunk size if not specified.
 *
 * \return The chunk size.
 */
int getChunkSize();

#endif // DISPATCHER_H
