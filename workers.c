#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <libgen.h>
#include <time.h>
#include <getopt.h>
#include <stdlib.h>


#include "workers.h"
#include "utils.h"
#include "utf8_utils.h"
#include "dispatcher.h"


// Function to process a chunk of data
ChunkResults processChunk(Chunk* chunk, int chunkSize) {
    ChunkResults results;
    

    int wordsCount = 0;
    int wordsWithConsonants = 0;

    
    results.fileIdx = chunk->fileIdx;
    results.wordsCount = wordsCount * chunk->fileIdx;
    results.wordsWithConsonants = wordsWithConsonants * chunk->fileIdx;

    return results;
        
}