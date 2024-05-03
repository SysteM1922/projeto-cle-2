#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include <math.h>
#include <getopt.h>

#define DISTRIBUTOR_RANK 0

/** \brief sort type */
int sortType = 0;

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
    int rank, size, opt, arraySize;
    char *fileName = NULL;
    FILE *file;
    int *array = NULL;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

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
        while ((opt = getopt(argc, argv, "f:h")) != -1)
        {
            switch (opt)
            {
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
        array = (int *)malloc(arraySize * sizeof(int)); /* allocate memory for the array */
        if (fread(array, sizeof(int), arraySize, file) != arraySize) /* read the array */
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
    }

    if (rank == DISTRIBUTOR_RANK)
    {
        printf("Time elapsed: %f s\n", get_delta_time());
    }

    MPI_Finalize();

    return EXIT_SUCCESS;
}