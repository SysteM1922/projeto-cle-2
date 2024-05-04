#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <getopt.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>

#include "dispatcher.h"
#include "workers.h"
#include "utils.h"
#include "utf8_utils.h"

char alphanumeric_chars_underscore[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '_'};

int alphanumeric_chars_underscore_array_size = sizeof(alphanumeric_chars_underscore) / sizeof(char);

char consonants[] = {'b', 'c', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'm', 'n', 'p', 'q', 'r', 's', 't', 'v', 'w', 'x', 'y', 'z'};

int consonants_array_size = sizeof(consonants) / sizeof(char);

char outside_word_chars[] = {0x20, 0x9, 0xD, 0xA, 0x2d, 0x22, 0x5b, 0x5d, 0x28, 0x29, 0x2e, 0x2c, 0x3a, 0x3b, 0x3f, 0x21};

int outside_word_array_size = sizeof(outside_word_chars) / sizeof(char);

static double get_delta_time(void);

int isLetter(wchar_t c) {
    return iswalnum(c) || c == L'_';
}

int isConsonant(wchar_t c) {
    return wcschr(L"bcdfghjklmnpqrstvwxyz", c) != NULL;
}

void extractLetter(wchar_t *letter) {
    *letter = towlower(*letter);
    wchar_t *simpleLetters = L"c";
    wchar_t *complexLetters = L"ç";
 
    for (int i = 0; i < wcslen(complexLetters); i++) {
        if (complexLetters[i] == *letter) {
            *letter = simpleLetters[i];
            break;
        }
    }
}

int main(int argc, char *argv[])
{

    setlocale(LC_ALL, "");

    int rank, nProcesses;

    MPI_Init(&argc, &argv);

    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);

    // Get the rank of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Check number of processes
    if (nProcesses < 2)
    {
        fprintf(stderr, "Error: This program requires at least 2 processes\n");
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    // start timer
    get_delta_time();

    int chunkSize = getChunkSize(argc, argv);
    // printf("Chunk size: %d\n", chunkSize);

    // if im the dispatcher -> Non-blocking communication
    if (rank == 0)
    {
        // printf("Number of processes: %d\n", nProcesses);
        // read the args; setup the files;  // Store information about files read (path and number of files)
        int numFiles = setupFiles(argc, argv);
        if (numFiles == -1)
        {
            fprintf(stderr, "Error setting up files\n");
            MPI_Finalize();
            return EXIT_FAILURE;
        }

        // Call setupDispatcher with the number of files
        if (setupDispatcher(numFiles, nProcesses) != 0)
        {
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
        ChunkResults *resultsArray[5000];
        MPI_Request *requests = calloc(5000 * 3, sizeof(MPI_Request));
        int nMessages = 0;

        do
        {
            for (int i = 1; i < nProcesses; i++)
            {
                bool work = true;

                // get the perfected chunk
                cChunk = getChunk();
                fileIdx = cChunk.fileIdx;
                printf("\n\nDispatcher: Got chunk %d, fileIdx: %d\n", cChunk.fileIdx, fileIdx);
                printf("Dispatcher: Chunk start position: %d\n", cChunk.startPosition);
                printf("Dispatcher: Chunk end position: %d\n", cChunk.endPosition);
                printf("Dispatcher: Chunk isFinal: %d\n", cChunk.isFinal);

                if (cChunk.isFinal || cChunk.startPosition == -1)
                {
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
                MPI_Send(cChunk.chunk, chunkSize, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);

                // Debug print: Sent chunk to worker
                printf("Dispatcher: Sent chunk to worker %d, fileIdx: %d\n", i, fileIdx);

                // receive results from worker i (non-blocking wa)
                ChunkResults *results = calloc(1, sizeof(ChunkResults));
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
                nMessages++;
            }

        } while (cChunk.startPosition != -1 || !cChunk.isFinal);

        // if i have no more chunks to give, i send a message to the workers to stop
        for (int i = 1; i < nProcesses; i++)
        {
            bool work = false;
            MPI_Send(&work, 1, MPI_C_BOOL, i, 0, MPI_COMM_WORLD);
            printf("Sent stop message to worker %d\n", i);
        }

        // not sure if this is needed
        // Validate all requests before calling MPI_Waitall
        for (int i = 0; i < nMessages * 3; i++)
        {
            if (requests[i] == MPI_REQUEST_NULL)
            {
                fprintf(stderr, "Invalid MPI_Request at index %d\n", i);
                MPI_Abort(MPI_COMM_WORLD, 1); // Abort if any request is invalid
            }
        }

        // Now call MPI_Waitall with the correct count
        MPI_Waitall(nMessages * 3, requests, MPI_STATUSES_IGNORE);

        // aggregate results
        for (int i = 0; i < nMessages; i++)
        {
            aggregateResults(resultsArray[i]->fileIdx, resultsArray[i]->wordsCount, resultsArray[i]->wordsWithConsonants);
        }

        // Print results
        printResults();

        printf("\n\nTime Elapsed: %f\n", get_delta_time());
    }

    // if im a worker
    else
    {
        bool work = true;

        do
        {
            // Receive a message with a flag to know if there's work to do
            MPI_Recv(&work, 1, MPI_C_BOOL, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if (work)
            {

                // receive the fileId, the start position, the end position and the chunk in different messages
                Chunk *receivedChunk = calloc(1, sizeof(Chunk));
                receivedChunk->chunk = calloc(chunkSize, sizeof(unsigned char));

                MPI_Recv(&receivedChunk->fileIdx, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&receivedChunk->startPosition, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&receivedChunk->endPosition, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(receivedChunk->chunk, chunkSize, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                // Debug print
                printf("Worker %d: Received chunk, fileIdx: %d\n", rank, receivedChunk->fileIdx);
                printf("Worker %d: Received chunk, start position: %d\n", rank, receivedChunk->startPosition);
                printf("Worker %d: Received chunk, end position: %d\n", rank, receivedChunk->endPosition);
                printf("ChunkSize: %d\n", chunkSize);

                ChunkResults results;
                // >>>>>>>>>>>>>>>>> Process the chunk <<<<<<<<<<<<<<<<<<<<

                int wordCount = 0;
                int inWord = 0;
                int matchWords = 0;
                wchar_t word[21];
                int word_len = 0;
                wchar_t c;
                int found = 0;

                for (int i = 0; i < chunkSize; i++)
                {
                    if (receivedChunk->chunk[i] == '\0')
                    {
                        break;
                    }
                    unsigned char currentByte = receivedChunk->chunk[i];
                    unsigned int currentChar = currentByte;

                    int bytesCount = numOfBytesInUTF8(currentByte); // Get the number of bytes for the current character

                    for (int j = 1; j < bytesCount; j++)
                    {
                        currentByte = receivedChunk->chunk[++i];
                        currentChar = (currentChar << 8) | currentByte; // Shift the current character and add the next byte
                    }

                    c = (wchar_t)currentChar;

                    if (isLetter(c))
                    {
                        if (found)
                        {
                            continue;
                        }
                        else
                        {
                            extractLetter(&c);
                            if (!inWord)
                            {
                                wordCount++;
                                inWord = 1;
                            }
                            if (isConsonant(c))
                            {
                                for (int i = 0; i < word_len; i++)
                                {
                                    if (word[i] == c)
                                    {
                                        found = 1;
                                        matchWords++;
                                        break;
                                    }
                                }
                                if (!found)
                                {
                                    word[word_len] = c;
                                    word_len++;
                                }
                            }
                        }
                    }
                    else if (c != L'’' && c != L'‘' && c != L'\'')
                    {
                        inWord = 0;
                        found = 0;
                        memset(word, '\0', sizeof(word));
                        word_len = 0;
                    }
                }

                // Send the results back to the dispatcher
                results.fileIdx = receivedChunk->fileIdx;
                results.wordsCount = wordCount;
                results.wordsWithConsonants = matchWords;

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

    MPI_Finalize();

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
    if (clock_gettime(CLOCK_MONOTONIC, &t1) != 0)
    {
        perror("clock_gettime");
        exit(1);
    }
    return (double)(t1.tv_sec - t0.tv_sec) + 1.0e-9 * (double)(t1.tv_nsec - t0.tv_nsec);
}