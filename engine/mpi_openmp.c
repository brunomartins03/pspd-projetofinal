#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#define ind2d(i, j) ((i) * (size + 2) + (j))

double wall_time(void) {
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);
    return (tv.tv_sec + tv.tv_usec / 1000000.0);
}

void generation(int * matrixIn, int * matrixOut, int size) {
    int i, j, vizviv;
    for (i = 1; i <= size; i++) {
        for (j = 1; j <= size; j++) {
            vizviv =
                matrixIn[ind2d(i - 1, j - 1)] + matrixIn[ind2d(i - 1, j)] +
                matrixIn[ind2d(i - 1, j + 1)] + matrixIn[ind2d(i, j - 1)] +
                matrixIn[ind2d(i, j + 1)] + matrixIn[ind2d(i + 1, j - 1)] +
                matrixIn[ind2d(i + 1, j)] + matrixIn[ind2d(i + 1, j + 1)];

            if (matrixIn[ind2d(i, j)] && vizviv < 2) matrixOut[ind2d(i, j)] = 0;
            else if (matrixIn[ind2d(i, j)] && vizviv > 3) matrixOut[ind2d(i, j)] = 0;
            else if (!matrixIn[ind2d(i, j)] && vizviv == 3) matrixOut[ind2d(i, j)] = 1;
            else matrixOut[ind2d(i, j)] = matrixIn[ind2d(i, j)];
        }
    }
}

void dump_matrix(int * matrix, int size, int first, int last, char * msg) {
    int i, ij;

    printf("%s; Dump posicoes [%d:%d, %d:%d] de tabuleiro %d x %d\n", msg, first, last, first, last, size, size);
    for (i = first; i <= last; i++) printf("=");
    printf("=\n");

    for (i = ind2d(first, 0); i <= ind2d(last, 0); i += ind2d(1, 0)) {
        for (ij = i + first; ij <= i + last; ij++) printf("%c", matrix[ij] ? 'X' : '.');
        printf("\n");
    }

    for (i = first; i <= last; i++) printf("=");
    printf("=\n");
}

void initialize_matrix(int* matrixIn, int* matrixOut, int size) {
    for (int ij = 0; ij < (size + 2) * (size + 2); ij++) {
        matrixIn[ij] = 0;
        matrixOut[ij] = 0;
    }

    matrixIn[ind2d(1, 2)] = 1;
    matrixIn[ind2d(2, 3)] = 1;
    matrixIn[ind2d(3, 1)] = 1;
    matrixIn[ind2d(3, 2)] = 1;
    matrixIn[ind2d(3, 3)] = 1;
}

int check_result(int * matrix, int size) {
    int cnt = 0;

    for (int ij = 0; ij < (size + 2) * (size + 2); ij++) cnt += matrix[ij];

    return (cnt == 5 &&
            matrix[ind2d(size - 2, size - 1)] &&
            matrix[ind2d(size - 1, size)] &&
            matrix[ind2d(size, size - 2)] &&
            matrix[ind2d(size, size - 1)] &&
            matrix[ind2d(size, size)]);
}

int main(int argc, char * argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Argumentos inválidos.\nUso: %s <powmin> <powmax>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int * matrixIn, * matrixOut;
    int powmin = atoi(argv[1]), powmax = atoi(argv[2]);
    char msg[9];
    double t0, t1, t2, t3;

    for (int pow = powmin; pow <= powmax; pow++) {
        int size = 1 << pow;

        t0 = wall_time();
        matrixIn  = (int*) malloc((size + 2) * (size + 2) * sizeof(int));
        matrixOut = (int*) malloc((size + 2) * (size + 2) * sizeof(int));
        initialize_matrix(matrixIn, matrixOut, size);
        t1 = wall_time();

        for (int i = 0; i < 2 * (size - 3); i++) {
            generation(matrixIn, matrixOut, size);
            generation(matrixOut, matrixIn, size);
        }
        t2 = wall_time();

        if (check_result(matrixIn, size)) printf("**RESULTADO CORRETO**\n");
        else printf("**RESULTADO ERRADO**\n");

        t3 = wall_time();

        printf("tam=%d; tempos: init=%7.7f, comp=%7.7f, fim=%7.7f, tot=%7.7f\n", size, t1 - t0, t2 - t1, t3 - t2, t3 - t0);
        free(matrixIn);
        free(matrixOut);
    }

    return 0;
}