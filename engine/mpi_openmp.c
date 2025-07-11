#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>
#include <omp.h>

#define ind2d(i, j) ((i) * (size + 2) + (j))
#define MASTER 0
#define TAG 0

typedef struct {
    int    pow;
    double t_init;
    double t_comp;
    double t_check;
    double t_total;
    int    correct;
} result_t;

double wall_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

void initialize_matrix(int *mat, int size) {
    mat[ind2d(1, 2)] = 1;
    mat[ind2d(2, 3)] = 1;
    mat[ind2d(3, 1)] = 1;
    mat[ind2d(3, 2)] = 1;
    mat[ind2d(3, 3)] = 1;
}

int check_result(int *mat, int size) {
    int cnt = 0;
    for (int ij = 0; ij < (size + 2)*(size + 2); ij++) cnt += mat[ij];
    int ok = (cnt == 5 &&
              mat[ind2d(size-2, size-1)] &&
              mat[ind2d(size-1, size)]   &&
              mat[ind2d(size,   size-2)] &&
              mat[ind2d(size,   size-1)] &&
              mat[ind2d(size,   size)]);
    return ok;
}

void run_simulation(int pow, result_t *res) {
    int size = 1 << pow;
    int  total_steps = 2*(size - 3);
    int *A = calloc((size+2)*(size+2), sizeof(int));
    int *B = calloc((size+2)*(size+2), sizeof(int));
    if (!A || !B) {
        fprintf(stderr, "Allocation failed for size=%d\n", size);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    double t0 = wall_time();
    initialize_matrix(A, size);
    double t1 = wall_time();

    #pragma omp parallel
    {
        int neighbors;
        for (int step = 0; step < total_steps; ++step) {
            #pragma omp for collapse(2) schedule(static) private(neighbors)
            for (int i = 1; i <= size; ++i) {
                for (int j = 1; j <= size; ++j) {
                    neighbors =
                        A[ind2d(i-1,j-1)] + A[ind2d(i-1,j)] + A[ind2d(i-1,j+1)] +
                        A[ind2d(i,  j-1)]                  + A[ind2d(i,  j+1)] +
                        A[ind2d(i+1,j-1)] + A[ind2d(i+1,j)] + A[ind2d(i+1,j+1)];
                    if (A[ind2d(i,j)] && (neighbors < 2 || neighbors > 3))
                        B[ind2d(i,j)] = 0;
                    else if (!A[ind2d(i,j)] && neighbors == 3)
                        B[ind2d(i,j)] = 1;
                    else
                        B[ind2d(i,j)] = A[ind2d(i,j)];
                }
            }
            #pragma omp barrier
            #pragma omp for collapse(2) schedule(static) private(neighbors)
            for (int i = 1; i <= size; ++i) {
                for (int j = 1; j <= size; ++j) {
                    neighbors =
                        B[ind2d(i-1,j-1)] + B[ind2d(i-1,j)] + B[ind2d(i-1,j+1)] +
                        B[ind2d(i,  j-1)]                  + B[ind2d(i,  j+1)] +
                        B[ind2d(i+1,j-1)] + B[ind2d(i+1,j)] + B[ind2d(i+1,j+1)];
                    if (B[ind2d(i,j)] && (neighbors < 2 || neighbors > 3))
                        A[ind2d(i,j)] = 0;
                    else if (!B[ind2d(i,j)] && neighbors == 3)
                        A[ind2d(i,j)] = 1;
                    else
                        A[ind2d(i,j)] = B[ind2d(i,j)];
                }
            }
            #pragma omp barrier
        }
    }

    double t2 = wall_time();
    int ok = check_result(A, size);
    double t3 = wall_time();

    res->pow     = pow;
    res->t_init  = t1 - t0;
    res->t_comp  = t2 - t1;
    res->t_check = t3 - t2;
    res->t_total = t3 - t0;
    res->correct = ok;

    free(A);
    free(B);
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (argc != 3) {
        if (rank == MASTER)
            fprintf(stderr, "Usage: %s <powmin> <powmax>\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    int powmin = atoi(argv[1]);
    int powmax = atoi(argv[2]);
    int nwork  = powmax - powmin + 1;

    if (nprocs != nwork + 1) {
        if (rank == MASTER) {
            fprintf(stderr,
                    "Error: expected %d worker ranks (pow=%d..%d), so mpirun -np %d\n",
                    nwork, powmin, powmax, nwork+1);
        }
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    if (rank != MASTER) {
        int pow = powmin + rank - 1;
        result_t myres;
        run_simulation(pow, &myres);
        MPI_Send(&myres, sizeof(myres), MPI_BYTE, MASTER, TAG, MPI_COMM_WORLD);
    } else {
        result_t *all = malloc(nwork * sizeof(result_t));
        for (int src = 1; src <= nwork; ++src) {
            MPI_Recv(&all[src-1], sizeof(result_t), MPI_BYTE,
                     src, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        printf(" pow |    init(s)  comp(s)    check(s)  total(s) | OK?\n");
        printf("-----+-------------------------------------------------\n");
        for (int i = 0; i < nwork; ++i) {
            result_t *r = &all[i];
            printf(" %3d | %9.6f  %9.6f  %9.6f  %9.6f |   %c\n",
                   r->pow,
                   r->t_init,
                   r->t_comp,
                   r->t_check,
                   r->t_total,
                   (r->correct ? 'Y' : 'N'));
        }
        free(all);
    }
    MPI_Finalize();
    return 0;
}