#ifndef WORKER_H
#define WORKER_H

#include "dispatcher.h"


typedef struct ChunkResults {

    int fileIdx;
    int wordsCount;              /**< Number of words processed by the thread for this chunk */
    int wordsWithConsonants;     /**< Number of words with at least two instances of the same consonant */

} ChunkResults;


#endif