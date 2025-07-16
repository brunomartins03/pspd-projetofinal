#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <omp.h>

#define ind2d(i, j) ((i) * (size + 2) + (j))

double wall_time(void) {
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);
    return (tv.tv_sec + tv.tv_usec / 1000000.0);
}

void run_generation(int * matrixIn, int * matrixOut, int size) {
    int alive_neigh;

    #pragma omp parallel for collapse(2) private(alive_neigh) schedule(static)
    for (int i = 1; i <= size; i++) {
        for (int j = 1; j <= size; j++) {
            alive_neigh =
                matrixIn[ind2d(i - 1, j - 1)] + matrixIn[ind2d(i - 1, j)] +
                matrixIn[ind2d(i - 1, j + 1)] + matrixIn[ind2d(i, j - 1)] +
                matrixIn[ind2d(i, j + 1)] + matrixIn[ind2d(i + 1, j - 1)] +
                matrixIn[ind2d(i + 1, j)] + matrixIn[ind2d(i + 1, j + 1)];

            if (matrixIn[ind2d(i, j)] && alive_neigh < 2) matrixOut[ind2d(i, j)] = 0;
            else if (matrixIn[ind2d(i, j)] && alive_neigh > 3) matrixOut[ind2d(i, j)] = 0;
            else if (!matrixIn[ind2d(i, j)] && alive_neigh == 3) matrixOut[ind2d(i, j)] = 1;
            else matrixOut[ind2d(i, j)] = matrixIn[ind2d(i, j)];
        }
    }
}

void matrix_init(int **matrixIn, int **matrixOut, int size) {
    *matrixIn = (int *) calloc((size + 2) * (size + 2), sizeof(int));
    *matrixOut = (int *) calloc((size + 2) * (size + 2), sizeof(int));
    (*matrixIn)[ind2d(1, 2)] = 1;
    (*matrixIn)[ind2d(2, 3)] = 1;
    (*matrixIn)[ind2d(3, 1)] = 1;
    (*matrixIn)[ind2d(3, 2)] = 1;
    (*matrixIn)[ind2d(3, 3)] = 1;
}

int check(int * matrix, int size) {
    int cnt = 0;

    #pragma omp parallel for reduction(+:cnt) schedule(static)
    for (int ij = 0; ij < (size + 2) * (size + 2); ij++)
        cnt += matrix[ij];

    return (cnt == 5 &&
            matrix[ind2d(size - 2, size - 1)] &&
            matrix[ind2d(size - 1, size)] &&
            matrix[ind2d(size, size - 2)] &&
            matrix[ind2d(size, size - 1)] &&
            matrix[ind2d(size, size)]);
}

void run(int * matrixIn, int * matrixOut, int matrix_size) {
    for (int i = 0; i < 2 * (matrix_size - 3); i++) {
        run_generation(matrixIn, matrixOut, matrix_size);
        run_generation(matrixOut, matrixIn, matrix_size);
    }
}

int main(int argc, char * argv[]) {
    int * matrixIn, * matrixOut;
    char msg[9];
    double t0, t1, t2, t3;
    int powmin, powmax;

    if (argc < 3) {
        fprintf(stderr, "Uso: %s <powmin> <powman> <nprocs> <nthreads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    powmin = atoi(argv[1]);
    powmax = atoi(argv[2]);

    for (int pow = powmin; pow <= powmax; pow++) {
        int matrix_size = 1 << pow;

        t0 = wall_time();
        matrix_init(&matrixIn, &matrixOut, matrix_size);
        t1 = wall_time();
        run(matrixIn, matrixOut, matrix_size);
        t2 = wall_time();
        if (check(matrixIn, matrix_size)) printf("**RESULTADO CORRETO**\n");
        else printf("**RESULTADO ERRADO**\n");
        t3 = wall_time();

        printf("tam=%d; tempos: init=%7.7f, comp=%7.7f, fim=%7.7f, tot=%7.7f\n",
               matrix_size, t1 - t0, t2 - t1, t3 - t2, t3 - t0);

        free(matrixIn);
        free(matrixOut);
    }

    return 0;
}