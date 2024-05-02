#include <stdio.h>
#include <stdlib.h>
// #include <time.h>
#include <linux/time.h>
#include <mpi.h>

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
    int rank, size;
    int i, j, k;
    int n = 1000;
    double *A, *B, *C;
    double t0, t1;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    A = (double *)malloc(n * n * sizeof(double));
    B = (double *)malloc(n * n * sizeof(double));
    C = (double *)malloc(n * n * sizeof(double));

    if (rank == 0)
    {
        for (i = 0; i < n * n; i++)
        {
            A[i] = 1.0;
            B[i] = 1.0;
            C[i] = 0.0;
        }
    }

    t0 = get_delta_time();

    MPI_Bcast(A, n * n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(B, n * n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    for (i = rank; i < n; i += size)
    {
        for (j = 0; j < n; j++)
        {
            for (k = 0; k < n; k++)
            {
                C[i * n + j] += A[i * n + k] * B[k * n + j];
            }
        }
    }

    MPI_Reduce(C, C, n * n, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    t1 = get_delta_time();

    if (rank == 0)
    {
        printf("Elapsed time: %f\n", t1 - t0);
    }

    free(A);
    free(B);
    free(C);

    MPI_Finalize();

    return 0;
}