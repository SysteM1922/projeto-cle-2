/**
 * \file main.c
 * 
 * \brief Main module.
 * 
 * This module provides the main program.
 * 
 * \author Guilherme Antunes - 103600
 * \author Pedro Rasinhas - 103541
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include <math.h>
#include <getopt.h>

#include "structs.h"
#include "bitonicSort.h"

#define DISTRIBUTOR_RANK 0 /* distributor rank */

/**
 * \brief Get the process time that has elapsed since last call of this time.
 *
 * \return process elapsed time
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
    return (double)(t1.tv_sec - t0.tv_sec) +
           1.0e-9 * (double)(t1.tv_nsec - t0.tv_nsec);
}

/**
 * \brief Main function.
 *
 * \param argc number of arguments
 * \param argv arguments
 *
 * \return exit status
 */
int main(int argc, char *argv[])
{
    int rank, size, opt, arraySize, i, nIteractions, nProcNow, sortType = 0; /* rank, size, opt, array size, i, number of iteractions, number of processes now, sort type */
    char *fileName = NULL; /* file name */
    FILE *file; /* file */
    int *sendArray = NULL, *recvArray; /* send array, recv array */
    int gMemb[8]; /* group members */

    WorkerArgs workerArgs; /* worker args */

    MPI_Comm presentComm, nextComm; /* present comm, next comm */
    MPI_Group presentGroup, nextGroup; /* present group, next group */

    MPI_Init(&argc, &argv); /* initialize the MPI environment */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); /* get the rank */
    MPI_Comm_size(MPI_COMM_WORLD, &size); /* get the size */

    nIteractions = log2(size);  /* calculate the number of iteractions */
    nProcNow = size; /* set the number of processes now */

    if (log2(size) != (int)log2(size)) /* check if the number of processes is a power of 2 */
    {
        if (rank == DISTRIBUTOR_RANK)
        {
            printf("Number of processes must be a power of 2!\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (size > 8) /* check if the number of processes is greater than 8 */
    {
        if (rank == DISTRIBUTOR_RANK)
        {
            printf("Number of processes must be less than or equal to 8!\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (rank == DISTRIBUTOR_RANK) /* check if the rank is the distributor rank */
    {
        while ((opt = getopt(argc, argv, "s:f:h")) != -1)
        {
            switch (opt)
            {
            case 's': /* sort type */
                sortType = atoi(optarg);
                break;
            case 'f': /* file name */
                fileName = optarg;
                break;
            case 'h': /* help */
                printf("Usage: mpiexec -n $n %s -f <file>\n", argv[0]);
                MPI_Finalize(); /* finalize the process */
                return EXIT_SUCCESS;
            default:
                printf("Usage: mpiexec -n $n %s -f <file>\n", argv[0]);
                MPI_Finalize(); /* finalize the process */
                return EXIT_FAILURE;
            }
        }

        (void)get_delta_time(); /* start timer */

        file = fopen(fileName, "rb");
        if (file == NULL) /* check if file was opened */
        {
            printf("Error opening file %s\n", fileName);
            MPI_Finalize(); /* finalize the process */
            return EXIT_FAILURE;
        }
        if (fread(&arraySize, sizeof(int), 1, file) != 1) /* read the size of the array */
        {
            printf("Error reading the size of the array\n");
            MPI_Finalize(); /* finalize the process */
            return EXIT_FAILURE;
        }
        sendArray = (int *)malloc(arraySize * sizeof(int)); /* allocate memory for the send array */
        recvArray = (int *)malloc(arraySize * sizeof(int)); /* allocate memory for the recv array */
        if (fread(sendArray, sizeof(int), arraySize, file) != arraySize) /* read the array */
        {
            printf("Error reading the array\n");
            MPI_Finalize(); /* finalize the process */
            return EXIT_FAILURE;
        }
        fclose(file);
        if (arraySize != pow(2, (int)log2(arraySize))) /* check if the array size is a power of 2 */
        {
            printf("Array size must be a power of 2!\n");
            MPI_Finalize(); /* finalize the process */
            return EXIT_FAILURE;
        }

        workerArgs.direction = sortType; /* set the sort type */
        workerArgs.size = arraySize;    /* set the array size */
    }

    MPI_Bcast(&workerArgs, sizeof(WorkerArgs), MPI_BYTE, DISTRIBUTOR_RANK, MPI_COMM_WORLD); /* broadcast the worker args */

    recvArray = (int *)malloc(workerArgs.size * sizeof(int)); /* allocate memory for the array */

    presentComm = MPI_COMM_WORLD; /* set the present comm to the world comm */
    MPI_Comm_group(presentComm, &presentGroup); /* get the group of the present comm */

    for (i = 0; i < size; i++) /* set the group members */
    {
        gMemb[i] = i;
    }

    for (i = 0; i < nIteractions + 1; i++) /* iterate through the number of iteractions planned */
    {
        if (i != 0)
        {
            MPI_Group_incl(presentGroup, nProcNow, gMemb, &nextGroup); /* include the next group */
            MPI_Comm_create(presentComm, nextGroup, &nextComm); /* create the next comm */
            presentGroup = nextGroup; /* set the present group to the next group */
            presentComm = nextComm; /* set the present comm to the next comm */

            if (rank >= nProcNow) /* check if the rank is greater than or equal to the number of processes now */
            {
                free(recvArray); /* free the recv array */
                MPI_Finalize(); /* finalize the process */
                return EXIT_SUCCESS;
            }
        }

        MPI_Comm_size(presentComm, &size); /* get the size of the present comm */
        MPI_Scatter(sendArray, workerArgs.size / size, MPI_INT, recvArray, workerArgs.size / size, MPI_INT, DISTRIBUTOR_RANK, presentComm); /* scatter the array */

        if (i == 0)
        {
            sort(recvArray, workerArgs.size / size, rank % 2 == workerArgs.direction); /* sort the array */
        }
        else
        {
            merge(recvArray, workerArgs.size / size, rank % 2 == workerArgs.direction); /* merge the array */
        }

        MPI_Gather(recvArray, workerArgs.size / size, MPI_INT, sendArray, workerArgs.size / size, MPI_INT, DISTRIBUTOR_RANK, presentComm); /* gather the array */

        nProcNow = nProcNow >> 1; /* shift the number of processes now */
    }

    if (rank == DISTRIBUTOR_RANK) /* check if the rank is the distributor rank */
    {
        validateArray(recvArray, arraySize, workerArgs.direction); /* validate if the array is correctly sorted */
        printf("Time elapsed: %f s\n", get_delta_time()); /* print the time elapsed */
    }

    free(recvArray); /* free the recv array */
    free(sendArray); /* free the send array */

    MPI_Finalize(); /* finalize the process */

    return EXIT_SUCCESS;
}