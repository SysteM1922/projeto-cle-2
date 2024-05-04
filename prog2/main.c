#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include <math.h>
#include <getopt.h>

#include "structs.h"
#include "bitonicSort.h"

#define DISTRIBUTOR_RANK 0

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

int main(int argc, char *argv[])
{
    int rank, size, opt, arraySize, i, nIteractions, nProcNow, sortType = 0;
    char *fileName = NULL;
    FILE *file;
    int *sendArray = NULL, *recvArray;
    int gMemb[8];

    WorkerArgs workerArgs;

    MPI_Comm presentComm, nextComm;
    MPI_Group presentGroup, nextGroup;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    nIteractions = log2(size);
    nProcNow = size;

    if (log2(size) != (int)log2(size))
    {
        if (rank == DISTRIBUTOR_RANK)
        {
            printf("Number of processes must be a power of 2!\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (size > 8)
    {
        if (rank == DISTRIBUTOR_RANK)
        {
            printf("Number of processes must be less than or equal to 8!\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    (void)get_delta_time(); /* start timer */

    if (rank == DISTRIBUTOR_RANK)
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
                MPI_Finalize();
                return EXIT_SUCCESS;
            default:
                printf("Usage: mpiexec -n $n %s -f <file>\n", argv[0]);
                MPI_Finalize();
                return EXIT_FAILURE;
            }
        }

        file = fopen(fileName, "rb");
        if (file == NULL) /* check if file was opened */
        {
            printf("Erro ao abrir o arquivo %s\n", fileName);
            MPI_Finalize();
            return EXIT_FAILURE;
        }
        if (fread(&arraySize, sizeof(int), 1, file) != 1) /* read the size of the array */
        {
            printf("Erro ao ler o tamanho do array\n");
            MPI_Finalize();
            return EXIT_FAILURE;
        }
        sendArray = (int *)malloc(arraySize * sizeof(int));              /* allocate memory for the array */
        recvArray = (int *)malloc(arraySize * sizeof(int));              /* allocate memory for the array */
        if (fread(sendArray, sizeof(int), arraySize, file) != arraySize) /* read the array */
        {
            printf("Erro ao ler o array\n");
            MPI_Finalize();
            return EXIT_FAILURE;
        }
        fclose(file);
        if (arraySize != pow(2, (int)log2(arraySize))) /* check if the array size is a power of 2 */
        {
            printf("Array size must be a power of 2!\n");
            MPI_Finalize();
            return EXIT_FAILURE;
        }

        workerArgs.direction = sortType;
        workerArgs.size = arraySize;
    }

    MPI_Bcast(&workerArgs, sizeof(WorkerArgs), MPI_BYTE, DISTRIBUTOR_RANK, MPI_COMM_WORLD);

    recvArray = (int *)malloc(workerArgs.size * sizeof(int)); /* allocate memory for the array */

    presentComm = MPI_COMM_WORLD;
    MPI_Comm_group(presentComm, &presentGroup);

    for (i = 0; i < size; i++)
    {
        gMemb[i] = i;
    }

    for (i = 0; i < nIteractions + 1; i++)
    {
        if (i != 0)
        {
            MPI_Group_incl(presentGroup, nProcNow, gMemb, &nextGroup);
            MPI_Comm_create(presentComm, nextGroup, &nextComm);
            presentGroup = nextGroup;
            presentComm = nextComm;

            if (rank >= nProcNow)
            {
                free(recvArray);
                MPI_Finalize();
                return EXIT_SUCCESS;
            }
        }

        MPI_Comm_size(presentComm, &size);
        MPI_Scatter(sendArray, workerArgs.size / size, MPI_INT, recvArray, workerArgs.size / size, MPI_INT, DISTRIBUTOR_RANK, presentComm);

        if (i == 0)
        {
            sort(recvArray, workerArgs.size / size, rank % 2 == workerArgs.direction);
        }
        else
        {
            merge(recvArray, workerArgs.size / size, rank % 2 == workerArgs.direction);
        }

        MPI_Gather(recvArray, workerArgs.size / size, MPI_INT, sendArray, workerArgs.size / size, MPI_INT, DISTRIBUTOR_RANK, presentComm);

        nProcNow = nProcNow >> 1;
    }

    if (rank == DISTRIBUTOR_RANK)
    {
        validateArray(recvArray, arraySize, workerArgs.direction);
        printf("Time elapsed: %f s\n", get_delta_time());
    }

    free(recvArray);
    free(sendArray);

    MPI_Finalize();

    return EXIT_SUCCESS;
}