#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <stdbool.h>
#include <stdio.h>

#define DEFAULT_CHUNK_SIZE 4096 // 4KB
#define MAX_PROCESSES 8
#define MAX_FILES 20
#define MAX_FILE_PATH_SIZE 128

typedef struct File{

    short currentFileIdx;
    FILE* currentFILE;
    int currentPosition;     /**< Current position in the file */
    int fileSize;            /**< Size of the file */
} File;

typedef struct FilesInfo {

    char* filenames[MAX_FILES]; // Array to store filenames
    int nFiles;

} FilesInfo;

typedef struct FileData {

    int fileIdx;
    int wordsCount;              /**< Number of words processed by the thread for this chunk */
    int wordsWithConsonants;     /**< Number of words with at least two instances of the same consonant */

} FileData;

typedef struct Chunk {
    unsigned char* chunk;       /**< Chunk of data read from the file */
    bool isFinal;
    int fileIdx;
    int startPosition;   	    /**< Start position of the chunk in the file */
    int endPosition;   	        /**< End position of the chunk in the file */
} Chunk;


int setupFiles(int argc, char *argv[]);
int setupDispatcher(int numFiles, int nProcesses);
Chunk getChunk();
void aggregateResults(int fileIdx, int words, int consonants);
void printResults();
int getChunkSize();
#endif
