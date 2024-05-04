#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>

#include "dispatcher.h"
#include "utf8_utils.h"
#include "utils.h"


static File* file;             // File structure - indicates the current file being read by a worker thread.
static FilesInfo* filesInfo;    // FilesInfo structure - contains the names of the files to be read and the amount of files to be read.  
static FileData* filesData;     // FileData structure - contains the information of a file.


int getChunkSize(int argc, char *argv[]) {
    int opt;
    int chunkSize = DEFAULT_CHUNK_SIZE; // Default chunk size

    // Parse command-line arguments
    while ((opt = getopt(argc, argv, "s:"))!= -1) {
        switch (opt) {
            case 's':
                chunkSize = atoi(optarg);
                if (chunkSize <= 0) {
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

    return chunkSize;
}

int parseArgs(int argc, char *argv[], FilesInfo *filesInfo) {
    int opt;
    int chunkSize = DEFAULT_CHUNK_SIZE; // Default chunk size

    // Initialize filesInfo
    filesInfo->nFiles = 0;

    // Parse command-line arguments
    while ((opt = getopt(argc, argv, "s:"))!= -1) {
        switch (opt) {
            case 's':
                chunkSize = atoi(optarg);
                if (chunkSize <= 0) {
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
    for (int i = optind; i < argc; i++) {
        filesInfo->filenames[filesInfo->nFiles++] = strdup(argv[i]);
    }

    return 0;
}

int setupFiles(int argc, char *argv[]) {
    // Allocate memory for filesInfo
    filesInfo = malloc(sizeof(FilesInfo));
    if (!filesInfo) {
        perror("Failed to allocate memory for filesInfo");
        return -1;
    }

    // Parse command-line arguments
    if (parseArgs(argc, argv, filesInfo)!= 0) {
        return -1;
    }

    return filesInfo->nFiles;
}

int setupDispatcher(int numFiles, int nProcesses) {
    // Allocate memory for file and FileData
    file = malloc(sizeof(File));
    if (!filesInfo) {
        perror("Failed to allocate memory for filesInfo");
        return -1;
    }

    filesData = calloc(nProcesses, sizeof(FileData));
    if (!filesData) {
        perror("Failed to allocate memory for filesData");
        return -1;
    }

    file->currentFileIdx = 0;
    file->currentFILE = NULL;
    file->currentPosition = 0;
    file->fileSize = getFileSize(filesInfo->filenames[file->currentFileIdx]);

    return 0;
}


Chunk getChunk() {
    Chunk chunk;
    chunk.chunk = malloc(DEFAULT_CHUNK_SIZE);
    chunk.fileIdx = file->currentFileIdx; // Get the current file index
    chunk.isFinal = false;
    chunk.startPosition = file->currentPosition;
    chunk.endPosition = file->currentPosition + DEFAULT_CHUNK_SIZE;
    
    long fileSize = getFileSize(filesInfo->filenames[file->currentFileIdx]);
    

    // check if we have read all the files
    // and if we have reached the end of the last file
    if (file->currentFileIdx >= filesInfo->nFiles && file->currentPosition >= fileSize) {
        chunk.isFinal = true;
        chunk.startPosition = -1;
        return chunk;
    }

    // if we have reached the end of the file, move on to the next file
    if (file->currentPosition >= fileSize) {
        file->currentFileIdx++;
        file->currentPosition = 0;
    }

    // if we are in the begining, or if we have moved to the next file
    if (file->currentFILE == NULL) {
        FILE* fileS = fopen(filesInfo->filenames[file->currentFileIdx], "r");
        if (fileS == NULL) {
            perror("Failed to open file");
            printf("\n\nfname: %s\n\n", filesInfo->filenames[file->currentFileIdx]);
        }
        file->currentFILE = fileS;
        file->currentPosition = 0;
    }

    FILE* fileS = file->currentFILE;

    int startPostion = file->currentPosition;
    int endPosition = file->currentPosition + DEFAULT_CHUNK_SIZE;
    int bitsRead = 0;
    unsigned int currentChar = 0;
    int lastSeparator = 0;


    while (!feof(fileS) && bitsRead < endPosition - startPostion) {
        unsigned char currentByte = fgetc(fileS);
        int bytesCount = numOfBytesInUTF8(currentByte);

        currentChar = currentByte;

        if (bitsRead + bytesCount <= endPosition - startPostion) {
            chunk.chunk[bitsRead++] = currentByte;
            for (int i = 1; i < bytesCount; i++) {
                currentByte = fgetc(fileS);
                chunk.chunk[bitsRead++] = currentByte;
                currentChar = currentChar << 8 | currentByte;
            }
        } else {
            fseek(fileS, -1, SEEK_CUR);
            break;
        }

        if (isSeparationCharacter(currentChar) || isWhiteSpace(currentChar) || isPunctuationMark(currentChar)) {
            lastSeparator = bitsRead;
        }
    }

    if (bitsRead >= endPosition - startPostion && (!isSeparationCharacter(currentChar) 
                                    && !isWhiteSpace(currentChar) && !isPunctuationMark(currentChar))) {
        fseek(fileS, lastSeparator - bitsRead, SEEK_CUR);

        unsigned int diff = bitsRead - lastSeparator;

        for (int i = 0; i < diff; i++) {
            chunk.chunk[--bitsRead] = 0x00;
            fseek(fileS, -1, SEEK_CUR);
        }
    }

    file->currentPosition += bitsRead;
    chunk.endPosition = file->currentPosition;

    if (feof(fileS)) {
        file->currentFileIdx++;
        fclose(fileS);
        file->currentFILE = NULL;
        file->currentPosition = 0;
    }

    if (endPosition > fileSize) {
        chunk.endPosition = fileSize;
    }

    // debug print
    // printf("Chunk: %s\n", chunk.chunk);
    printf("\n\nFileIdx: %d\n", chunk.fileIdx);
    printf("FileSize: %ld\n", fileSize);
    printf("StartPosition: %d\n", chunk.startPosition);
    printf("EndPosition: %d\n", chunk.endPosition);
    printf("Chunk: %s\n", chunk.isFinal ? "true" : "false");

    return chunk;
}

void aggregateResults(int fileIdx, int words, int consonants) {
    filesData[fileIdx].fileIdx = fileIdx;
    filesData[fileIdx].wordsCount += words;
    filesData[fileIdx].wordsWithConsonants += consonants;
}

void printResults() {
    for (int i = 0; i < filesInfo->nFiles; i++) {
        printf("FileIdx: %d\n", filesData[i].fileIdx);
        printf("WordsCount: %d\n", filesData[i].wordsCount);
        printf("WordsWithConsonants: %d\n", filesData[i].wordsWithConsonants);
    }
}