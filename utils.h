

#ifndef UTILS_H
#define UTILS_H
#include <mpi.h>
#include "dispatcher.h"
#include "workers.h"

void sendChunk(Chunk* chunk, int dest);

long getFileSize(const char *fileName);
#endif