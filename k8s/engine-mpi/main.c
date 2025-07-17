#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>
#include <omp.h>

#define ind2d(i, j, tam) ((i) * (tam + 2) + (j))
#define POWMIN 3
#define POWMAX 10

double wall_time(void)
{
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);
    return (tv.tv_sec + tv.tv_usec / 1000000.0);
}

void UmaVida(int *tabulIn, int *tabulOut, int tam, int loc_lines)
{
    int vizviv;

    #pragma omp parallel for collapse(2) schedule(static) private(vizviv)
    for (int i = 1; i <= loc_lines; i++)
    {
        for (int j = 1; j <= tam; j++)
        {
            vizviv = tabulIn[ind2d(i - 1, j - 1, tam)] + tabulIn[ind2d(i - 1, j, tam)] +
                     tabulIn[ind2d(i - 1, j + 1, tam)] + tabulIn[ind2d(i, j - 1, tam)] +
                     tabulIn[ind2d(i, j + 1, tam)] + tabulIn[ind2d(i + 1, j - 1, tam)] +
                     tabulIn[ind2d(i + 1, j, tam)] + tabulIn[ind2d(i + 1, j + 1, tam)];

            if (tabulIn[ind2d(i, j, tam)] && vizviv < 2)
                tabulOut[ind2d(i, j, tam)] = 0;
            else if (tabulIn[ind2d(i, j, tam)] && vizviv > 3)
                tabulOut[ind2d(i, j, tam)] = 0;
            else if (!tabulIn[ind2d(i, j, tam)] && vizviv == 3)
                tabulOut[ind2d(i, j, tam)] = 1;
            else
                tabulOut[ind2d(i, j, tam)] = tabulIn[ind2d(i, j, tam)];
        }
    }
}

void InitTabul(int *tabul, int tam, int loc_lines, int rank)
{
    for (int ij = 0; ij < (loc_lines + 2) * (tam + 2); ij++)
    {
        tabul[ij] = 0;
    }
    if (rank == 0)
    {
        tabul[ind2d(1, 2, tam)] = 1;
        tabul[ind2d(2, 3, tam)] = 1;
        tabul[ind2d(3, 1, tam)] = 1;
        tabul[ind2d(3, 2, tam)] = 1;
        tabul[ind2d(3, 3, tam)] = 1;
    }
}

int main(int argc, char *argv[])
{
    int pow, tam, nprocs, rank;
    double t0, t1, t2, t3;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (pow = POWMIN; pow <= POWMAX; pow++)
    {
        tam = 1 << pow;
        int loc_lines = tam / nprocs;

        if (tam % nprocs != 0)
        {
            if (rank == 0)
                printf("Tamanho %d não divisível por %d processos.\n", tam, nprocs);
            MPI_Finalize();
            return 1;
        }

        int *tabulIn = (int *)malloc((loc_lines + 2) * (tam + 2) * sizeof(int));
        int *tabulOut = (int *)malloc((loc_lines + 2) * (tam + 2) * sizeof(int));

        t0 = wall_time();
        InitTabul(tabulIn, tam, loc_lines, rank);

        t1 = wall_time();

        for (int iter = 0; iter < 2 * (tam - 3); iter++)
        {
            // Troca com o vizinho de cima
            if (rank != 0)
                MPI_Sendrecv(&tabulIn[ind2d(1, 0, tam)], tam + 2, MPI_INT, rank - 1, 0,
                             &tabulIn[ind2d(0, 0, tam)], tam + 2, MPI_INT, rank - 1, 0,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // Troca com o vizinho de baixo
            if (rank != nprocs - 1)
                MPI_Sendrecv(&tabulIn[ind2d(loc_lines, 0, tam)], tam + 2, MPI_INT, rank + 1, 0,
                             &tabulIn[ind2d(loc_lines + 1, 0, tam)], tam + 2, MPI_INT, rank + 1, 0,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            UmaVida(tabulIn, tabulOut, tam, loc_lines);

            // Troca ponteiros
            int *tmp = tabulIn;
            tabulIn = tabulOut;
            tabulOut = tmp;
        }

        t2 = wall_time();

        if (rank == 0)
            printf("tam=%d; tempos: init=%.7f, comp=%.7f, total=%.7f\n", tam, t1 - t0, t2 - t1, t2 - t0);

        free(tabulIn);
        free(tabulOut);
    }

    MPI_Finalize();
    return 0;
}
