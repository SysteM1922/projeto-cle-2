#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <getopt.h>

#include "dispatcher.h"
#include "workers.h"
#include "utils.h"
#include "utf8_utils.h"


static double get_delta_time(void);
int main(int argc, char *argv[]) {

    int rank, nProcesses;


    MPI_Init(&argc, &argv);

    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);

    // Get the rank of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Check number of processes
    if (nProcesses < 2) {
        fprintf(stderr, "Error: This program requires at least 2 processes\n");
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    // start timer 
    get_delta_time();

    // if im the dispatcher -> Non-blocking communication
    if (rank == 0) {
        // printf("Number of processes: %d\n", nProcesses);
        // read the args; setup the files;  // Store information about files read (path and number of files)
        int numFiles = setupFiles(argc, argv);
        if (numFiles == -1) {
            fprintf(stderr, "Error setting up files\n");
            MPI_Finalize();
            return EXIT_FAILURE;
        }

        // Call setupDispatcher with the number of files
        if (setupDispatcher(numFiles, nProcesses)!= 0) {
            fprintf(stderr, "Error setting up dispatcher\n");
            MPI_Finalize();
            return EXIT_FAILURE;
        }

        printf("SETUP TIME: %f\n\n\n", get_delta_time());

        // --------------- Actual work ---------------

        // while i have chunks to give to the workers
        // I get the chunks (perfected) and give them to the workers

        Chunk cChunk;
        int fileIdx;
        ChunkResults* resultsArray[5000];
        MPI_Request* requests = calloc(5000*3, sizeof(MPI_Request));
        int nMessages = 0;

        do {
            for (int i = 1; i < nProcesses; i++) {
                bool work = true;

                // get the perfected chunk
                cChunk = getChunk();
                fileIdx = cChunk.fileIdx;
                printf("\n\nDispatcher: Got chunk %d, fileIdx: %d\n", cChunk.fileIdx, fileIdx);
                printf("Dispatcher: Chunk start position: %d\n", cChunk.startPosition);
                printf("Dispatcher: Chunk end position: %d\n", cChunk.endPosition);
                printf("Dispatcher: Chunk isFinal: %d\n", cChunk.isFinal);

                if (cChunk.isFinal || cChunk.startPosition == -1) {
                    break;
                }

                // send chunk to worker i
                // send a message to worker i with a flag to know if i have work to do (work)
                MPI_Send(&work, 1, MPI_C_BOOL, i, 0, MPI_COMM_WORLD);

                printf("Dispatcher: Sent work flag to worker %d\n", i);

                // send the actual chunk to worker i
                // must send the fileIdx and then the chunk
                MPI_Send(&cChunk.fileIdx, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                MPI_Send(&cChunk.startPosition, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                MPI_Send(&cChunk.endPosition, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                MPI_Send(cChunk.chunk, DEFAULT_CHUNK_SIZE, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);

                // Debug print: Sent chunk to worker
                printf("Dispatcher: Sent chunk to worker %d, fileIdx: %d\n", i, fileIdx);


                // receive results from worker i (non-blocking wa)
                ChunkResults* results = calloc(1, sizeof(ChunkResults));
                int currentIndex = nMessages * 3;
                MPI_Irecv(&results->fileIdx, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &requests[currentIndex]);
                MPI_Irecv(&results->wordsCount, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &requests[currentIndex + 1]);
                MPI_Irecv(&results->wordsWithConsonants, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &requests[currentIndex + 2]);

                // Debug print: Deserialized results
                printf("Dispatcher: Received results from worker %d\n", i);
                printf("Dispatcher: Results for fileIdx: %d, wordsCount: %d, wordsWithConsonants: %d\n", results->fileIdx, results->wordsCount, results->wordsWithConsonants);
                
                // store results
                // must store the results to an array of ChunkResults
                resultsArray[nMessages] = results;


                // Debug print: Stored results
                printf("Dispatcher: Stored results for fileIdx: %d, worker: %d\n", results->fileIdx, i);
                nMessages++ ; 
            }

        } 
        while (cChunk.startPosition != -1 || !cChunk.isFinal);

        // if i have no more chunks to give, i send a message to the workers to stop
        for (int i = 1; i < nProcesses; i++) {
            bool work = false;
            MPI_Send(&work, 1, MPI_C_BOOL, i, 0, MPI_COMM_WORLD);
            printf("Sent stop message to worker %d\n", i);
        }


        // not sure if this is needed
        // Validate all requests before calling MPI_Waitall
        for (int i = 0; i < nMessages * 3; i++) {
            if (requests[i] == MPI_REQUEST_NULL) {
                fprintf(stderr, "Invalid MPI_Request at index %d\n", i);
                MPI_Abort(MPI_COMM_WORLD, 1); // Abort if any request is invalid
            }
        }

        // Now call MPI_Waitall with the correct count
        MPI_Waitall(nMessages * 3, requests, MPI_STATUSES_IGNORE);

        // aggregate results
        for (int i = 0; i < nMessages; i++) {
            aggregateResults(resultsArray[i]->fileIdx, resultsArray[i]->wordsCount, resultsArray[i]->wordsWithConsonants);
        }
        
        // Print results
        printResults();

        printf("\n\nTime Elapsed: %f\n", get_delta_time());

    }
    
    // if im a worker
    else {
        bool work = true;

        do {
            // Receive a message with a flag to know if there's work to do
            MPI_Recv(&work, 1, MPI_C_BOOL, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if (work) {

                // receive the fileId, the start position, the end position and the chunk in different messages
                Chunk* receivedChunk = calloc(1, sizeof(Chunk));
                receivedChunk->chunk = calloc(DEFAULT_CHUNK_SIZE, sizeof(unsigned char));

                MPI_Recv(&receivedChunk->fileIdx, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&receivedChunk->startPosition, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&receivedChunk->endPosition, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(receivedChunk->chunk, DEFAULT_CHUNK_SIZE, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                // Debug print
                // printf("Worker %d: Received chunk, fileIdx: %d\n", rank, fileId);
                // printf("Worker %d: Received chunk, start position: %d\n", rank, startPosition);
                // printf("Worker %d: Received chunk, end position: %d\n", rank, endPosition);
                // printf("Chunk: %s\n", chunk);

                ChunkResults results;
                // >>>>>>>>>>>>>>>>> Process the chunk <<<<<<<<<<<<<<<<<<<<

                int wordsCount = 69 * receivedChunk->fileIdx;
                int wordsWithConsonants = 420 * receivedChunk->fileIdx;
                
                results.fileIdx = receivedChunk->fileIdx;
                results.wordsCount = wordsCount;
                results.wordsWithConsonants = wordsWithConsonants;

                // send the results back to the dispatcher
                MPI_Send(&results.fileIdx, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                MPI_Send(&results.wordsCount, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                MPI_Send(&results.wordsWithConsonants, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

                // Debug print
                printf("Worker %d: Processed chunk and Sending results back to dispatcher, fileIdx: %d, wordsCount: %d, wordsWithConsonants: %d\n", rank, results.fileIdx, results.wordsCount, results.wordsWithConsonants);

            }
        } while (work);

        // Debug print
        printf("Worker %d: Finished processing chunks\n", rank);
    }

    MPI_Finalize ();

    return EXIT_SUCCESS;
}

/**
 *  \brief Get the process time that has elapsed since last call of this time.
 *
 *  \return process elapsed time
 */
static double get_delta_time(void)
{
    static struct timespec t0, t1;

    t0 = t1;
    if(clock_gettime (CLOCK_MONOTONIC, &t1) != 0) {
        perror("clock_gettime");
        exit(1);
    }
    return (double) (t1.tv_sec - t0.tv_sec) + 1.0e-9 * (double) (t1.tv_nsec - t0.tv_nsec);
}