#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>
#include <omp.h>

#define ind2d(i, j, size) ((i) * (size + 2) + (j))

double wall_time(void) {
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);
    return (tv.tv_sec + tv.tv_usec / 1000000.0);
}

void one_generation(int * matrixIn, int * matrixOut, int size, int loc_lines) {
    int neighbors_alive;

    #pragma omp parallel for collapse(2) schedule(static) private(neighbors_alive)
    for (int i = 1; i <= loc_lines; i++) {
        for (int j = 1; j <= size; j++) {
            neighbors_alive = matrixIn[ind2d(i - 1, j - 1, size)] + matrixIn[ind2d(i - 1, j, size)] +
                     matrixIn[ind2d(i - 1, j + 1, size)] + matrixIn[ind2d(i, j - 1, size)] +
                     matrixIn[ind2d(i, j + 1, size)] + matrixIn[ind2d(i + 1, j - 1, size)] +
                     matrixIn[ind2d(i + 1, j, size)] + matrixIn[ind2d(i + 1, j + 1, size)];

            if (matrixIn[ind2d(i, j, size)] && neighbors_alive < 2) matrixOut[ind2d(i, j, size)] = 0;
            else if (matrixIn[ind2d(i, j, size)] && neighbors_alive > 3) matrixOut[ind2d(i, j, size)] = 0;
            else if (!matrixIn[ind2d(i, j, size)] && neighbors_alive == 3) matrixOut[ind2d(i, j, size)] = 1;
            else matrixOut[ind2d(i, j, size)] = matrixIn[ind2d(i, j, size)];
        }
    }
}

void matrix_init(int ** matrixIn, int ** matrixOut, int size, int loc_lines, int rank) {
    *matrixIn = (int *) calloc((loc_lines + 2) * (size + 2), sizeof(int));
    *matrixOut = (int *) calloc((loc_lines + 2) * (size + 2), sizeof(int));

    if (rank == 0) {
        (*matrixIn)[ind2d(1, 2, size)] = 1;
        (*matrixIn)[ind2d(2, 3, size)] = 1;
        (*matrixIn)[ind2d(3, 1, size)] = 1;
        (*matrixIn)[ind2d(3, 2, size)] = 1;
        (*matrixIn)[ind2d(3, 3, size)] = 1;
    }
}

int main(int argc, char *argv[]) {
    int size, nprocs, rank, powmin, powmax;
    int * matrixIn, * matrixOut;
    double t0, t1, t2, t3;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc < 3) {
        if (rank == 0) {
            fprintf(stderr, "Uso: %s <powmin> <powmax>\n", argv[0]);
        }
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    powmin = atoi(argv[1]);
    powmax = atoi(argv[2]);

    for (int pow = powmin; pow <= powmax; pow++) {
        size = 1 << pow;
        int loc_lines = size / nprocs;

        if (size % nprocs != 0) {
            if (rank == 0) printf("Tamanho %d não divisível por %d processos.\n", size, nprocs);
            MPI_Finalize();
            return 1;
        }

        t0 = wall_time();
        matrix_init(&matrixIn, &matrixOut, size, loc_lines, rank);
        t1 = wall_time();

        for (int iter = 0; iter < 2 * (size - 3); iter++) {
            if (rank != 0)
                MPI_Sendrecv(&matrixIn[ind2d(1, 0, size)], size + 2, MPI_INT, rank - 1, 0,
                             &matrixIn[ind2d(0, 0, size)], size + 2, MPI_INT, rank - 1, 0,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if (rank != nprocs - 1)
                MPI_Sendrecv(&matrixIn[ind2d(loc_lines, 0, size)], size + 2, MPI_INT, rank + 1, 0,
                             &matrixIn[ind2d(loc_lines + 1, 0, size)], size + 2, MPI_INT, rank + 1, 0,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            one_generation(matrixIn, matrixOut, size, loc_lines);

            int * tmp = matrixIn;
            matrixIn = matrixOut;
            matrixOut = tmp;
        }

        t2 = wall_time();

        if (rank == 0)
            printf("tam=%d; tempos: init=%.7f, comp=%.7f, total=%.7f\n", size, t1 - t0, t2 - t1, t2 - t0);

        free(matrixIn);
        free(matrixOut);
    }

    MPI_Finalize();
    return 0;
}
