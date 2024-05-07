/**
 * \file dispatcher.c
 * \brief Dispatcher program for processing files in chunks.
 *
 * This file contains the main logic for reading files in chunks, processing them,
 * and aggregating results. It includes functions for parsing command-line arguments,
 * setting up file structures, dispatching tasks, and printing results.
 *
 * \author Pedro Rasinhas - 103541
 * \author Guilherme Antunes - 103600
 */

#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>

#include "dispatcher.h"
#include "utf8_utils.h"
#include "utils.h"

static File *file;                         // File structure - indicates the current file being read by a worker thread.
static FilesInfo *filesInfo;               // FilesInfo structure - contains the names of the files to be read and the amount of files to be read.
static FileData *filesData;                // FileData structure - contains the information of a file.
static int chunkSize = DEFAULT_CHUNK_SIZE; // Default chunk size

/**
 * \brief Parses command-line arguments to get the chunk size.
 *
 * \param argc The number of command-line arguments.
 * \param argv The array of command-line arguments.
 * \return The chunk size specified by the user, or the default chunk size if not specified.
 */
int getChunkSize(int argc, char *argv[])
{
    int opt;

    // Parse command-line arguments
    while ((opt = getopt(argc, argv, "s:")) != -1)
    {
        switch (opt)
        {
        case 's':
            chunkSize = atoi(optarg);
            if (chunkSize <= 0)
            {
                fprintf(stderr, "Chunk size must be a positive integer\n");
                return -1;
            }
            break;
        case '?':
            printf("invalid option\n");
            printf("Usage: %s -s <size_chunk> <filename1> <filename2> ...\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (chunkSize == 4096)
    {
        chunkSize = chunkSize + 4;
    }

    return chunkSize;
}

/**
 * \brief Parses command-line arguments to extract file names and chunk size.
 *
 * \param argc The number of command-line arguments.
 * \param argv The array of command-line arguments.
 * \param filesInfo A pointer to the FilesInfo structure to store file names and count.
 * \return 0 on success, -1 if an error occurs (e.g., invalid chunk size).
 */
int parseArgs(int argc, char *argv[], FilesInfo *filesInfo)
{
    int opt;
    int chunkSize = DEFAULT_CHUNK_SIZE; // Default chunk size

    // Initialize filesInfo
    filesInfo->nFiles = 0;

    // Parse command-line arguments
    while ((opt = getopt(argc, argv, "s:")) != -1)
    {
        switch (opt)
        {
        case 's':
            chunkSize = atoi(optarg);
            if (chunkSize <= 0)
            {
                fprintf(stderr, "Chunk size must be a positive integer\n");
                return -1;
            }
            break;
        case '?':
            printf("invalid option\n");
            printf("Usage: %s -s <size_chunk> <filename1> <filename2> ...\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    // Store filenames
    for (int i = optind; i < argc; i++)
    {
        filesInfo->filenames[filesInfo->nFiles++] = strdup(argv[i]);
    }

    return 0;
}

/**
 * \brief Sets up the necessary structures for file processing.
 *
 * Allocates memory for the FilesInfo structure and parses command-line arguments.
 *
 * \param argc The number of command-line arguments.
 * \param argv The array of command-line arguments.
 * \return The number of files to be processed, or -1 on error.
 */
int setupFiles(int argc, char *argv[])
{
    // Allocate memory for filesInfo
    filesInfo = malloc(sizeof(FilesInfo));
    if (!filesInfo)
    {
        perror("Failed to allocate memory for filesInfo");
        return -1;
    }

    // Parse command-line arguments
    if (parseArgs(argc, argv, filesInfo) != 0)
    {
        return -1;
    }

    return filesInfo->nFiles;
}

/**
 * \brief Initializes the dispatcher with the number of files and processes.
 *
 * Allocates memory for the File and FileData structures and initializes them.
 *
 * \param numFiles The number of files to be processed.
 * \param nProcesses The number of processes to be used for processing.
 * \return 0 on success, -1 on error.
 */
int setupDispatcher(int numFiles, int nProcesses)
{
    // Allocate memory for file and FileData
    file = malloc(sizeof(File));
    if (!filesInfo)
    {
        perror("Failed to allocate memory for filesInfo");
        return -1;
    }

    filesData = calloc(nProcesses, sizeof(FileData));
    if (!filesData)
    {
        perror("Failed to allocate memory for filesData");
        return -1;
    }

    file->currentFileIdx = 0;
    file->currentFILE = NULL;
    file->currentPosition = 0;
    file->fileSize = getFileSize(filesInfo->filenames[file->currentFileIdx]);

    return 0;
}

/**
 * \brief Aggregates results from file processing.
 *
 * Updates the FileData structure with the number of words and words with consonants.
 *
 * \param fileIdx The index of the file being processed.
 * \param words The number of words found in the file.
 * \param consonants The number of words with consonants found in the file.
 */
void aggregateResults(int fileIdx, int words, int consonants)
{
    filesData[fileIdx].fileIdx = fileIdx;
    filesData[fileIdx].wordsCount += words;
    filesData[fileIdx].wordsWithConsonants += consonants;
}

/**
 * \brief Prints the aggregated results for each file.
 *
 * Outputs the file index, word count, and words with consonants for each file.
 */
void printResults()
{
    for (int i = 0; i < filesInfo->nFiles; i++)
    {
        printf("\nFileIdx: %d\n", filesData[i].fileIdx);
        printf("WordsCount: %d\n", filesData[i].wordsCount);
        printf("WordsWithConsonants: %d\n", filesData[i].wordsWithConsonants);
    }
}

/**
 * \brief Retrieves the next chunk of data from the current file.
 *
 * Reads a chunk of data from the current file, handling file boundaries and UTF-8 encoding.
 *
 * \return A Chunk structure containing the chunk data, file index, and position information.
 */
Chunk getChunk()
{
    Chunk chunk;
    chunk.chunk = calloc(chunkSize, sizeof(unsigned char));
    chunk.fileIdx = file->currentFileIdx; // Get the current file index
    chunk.isFinal = false;
    chunk.startPosition = file->currentPosition;
    chunk.endPosition = file->currentPosition + chunkSize;

    long fileSize = getFileSize(filesInfo->filenames[file->currentFileIdx]);

    // check if we have read all the files
    // and if we have reached the end of the last file
    if (file->currentFileIdx >= filesInfo->nFiles)
    {
        chunk.isFinal = true;
        chunk.startPosition = -1;
        return chunk;
    }

    // if we have reached the end of the file, move on to the next file
    if (file->currentPosition >= fileSize)
    {
        file->currentFileIdx++;
        file->currentPosition = 0;
    }

    // if we are in the begining, or if we have moved to the next file
    if (file->currentFILE == NULL)
    {
        FILE *fileS = fopen(filesInfo->filenames[file->currentFileIdx], "rb");
        if (fileS == NULL)
        {
            perror("Failed to open file");
            printf("\n\nfname: %s\n\n", filesInfo->filenames[file->currentFileIdx]);
        }
        file->currentFILE = fileS;
        file->currentPosition = 0;
    }

    FILE *fileS = file->currentFILE;

    int endPosition = file->currentPosition + chunkSize;
    int bitsRead = 0;

    int bytesRead  = fread(chunk.chunk, sizeof(unsigned char), chunkSize, fileS);

    if (!feof(fileS)) {
        // check if last char is in the middle of a utf8 char
        while ((chunk.chunk[bytesRead - 1] & 0xC0) == 0x80)
        {   
            bytesRead--;
            fseek(fileS, -1, SEEK_CUR);
            
        }

        // check if last char is in the middle of a word
        while(isLetter(chunk.chunk[bytesRead - 1]) || chunk.chunk[bytesRead - 1] == L'’' || chunk.chunk[bytesRead - 1] == L'‘' || chunk.chunk[bytesRead - 1] == L'\'')
        {
            bytesRead--;
            fseek(fileS, -1, SEEK_CUR);
        }

        memset(chunk.chunk + bytesRead, 0, chunkSize - bytesRead);

        chunk.chunk[bytesRead] = '\0';
        bitsRead = bytesRead;


    }
    
    // printf("WordCount: %d\n", wordCount);

    file->currentPosition += bitsRead;
    chunk.endPosition = file->currentPosition;

    if (feof(fileS))
    {
        file->currentFileIdx++;
        fclose(fileS);
        file->currentFILE = NULL;
        file->currentPosition = 0;
    }

    if (endPosition > fileSize)
    {
        chunk.endPosition = fileSize;
    }

    // debug print
    // printf("Chunk: %s\n", chunk.chunk);
    // printf("\n\nFileIdx: %d\n", chunk.fileIdx);
    // printf("FileSize: %ld\n", fileSize);
    // printf("StartPosition: %d\n", chunk.startPosition);
    // printf("EndPosition: %d\n", chunk.endPosition);
    // printf("Chunk: %s\n", chunk.isFinal ? "true" : "false");

    return chunk;
}
