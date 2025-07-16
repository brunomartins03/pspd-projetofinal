#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <mpi.h>
#include <omp.h>
#include <unistd.h>
#include <time.h>

#define ind2d(i, j) ((i) * (size + 2) + (j))
#define MASTER 0
#define TAG     0

typedef struct {
    char    request_id[64];
    char    client_id[MPI_MAX_PROCESSOR_NAME];
    char    engine[64];
    int     powmin, powmax;
    char    start_time[30];
    char    end_time[30];
    double  duration_ms;
    char    status[8];
    char    error_message[128];
    int     num_generations;
    int     board_size;
    char    host_node[64];
    int     num_clients_active;
    char    timestamp[30];
} result_t;

double wall_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

void fmt_time_iso(time_t t, char *buf) {
    struct tm tm;
    gmtime_r(&t, &tm);
    strftime(buf, 30, "%Y-%m-%dT%H:%M:%SZ", &tm);
}

void initialize_matrix(int *A, int size) {
    A[ind2d(1,2)] = 1;
    A[ind2d(2,3)] = 1;
    A[ind2d(3,1)] = 1;
    A[ind2d(3,2)] = 1;
    A[ind2d(3,3)] = 1;
}

int check_result(int *A, int size) {
    int cnt = 0;
    for (int i = 0; i < (size + 2) * (size + 2); i++)
        cnt += A[i];
    int ok = (cnt == 5 &&
              A[ind2d(size-2,size-1)] &&
              A[ind2d(size-1,size)] &&
              A[ind2d(size,size-2)] &&
              A[ind2d(size,size-1)] &&
              A[ind2d(size,size)]);
    return ok;
}

void run_simulation(int pow, int powmin, int powmax, int num_clients, int rank, result_t *r) {
    int size = 1 << pow;
    int steps = 2 * (size - 3);
    int *A = calloc((size + 2) * (size + 2), sizeof(int));
    int *B = calloc((size + 2) * (size + 2), sizeof(int));
    if (!A || !B) {
        perror("calloc");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);
    snprintf(r->request_id, sizeof(r->request_id), "%ld%06ld-r%d",
             (long)tv.tv_sec, (long)tv.tv_usec, rank);
    gethostname(r->host_node, sizeof(r->host_node));
    int namelen;
    MPI_Get_processor_name(r->client_id, &namelen);
    strncpy(r->engine, "game_of_life_mpi_omp", sizeof(r->engine));
    r->powmin = powmin;
    r->powmax = powmax;
    r->num_generations = steps;
    r->board_size = size;
    r->num_clients_active = num_clients;

    time_t ts0 = time(NULL);
    fmt_time_iso(ts0, r->start_time);

    double t0 = wall_time();
    initialize_matrix(A, size);
    double t1 = wall_time();

    #pragma omp parallel
    {
        int neigh;
        for (int st = 0; st < steps; st++) {
            #pragma omp for collapse(2) private(neigh) schedule(static)
            for (int i = 1; i <= size; i++)
                for (int j = 1; j <= size; j++) {
                    neigh = A[ind2d(i-1,j-1)] + A[ind2d(i-1,j)] + A[ind2d(i-1,j+1)] +
                            A[ind2d(i,j-1)] + A[ind2d(i,j+1)] +
                            A[ind2d(i+1,j-1)] + A[ind2d(i+1,j)] + A[ind2d(i+1,j+1)];
                    B[ind2d(i,j)] = (neigh == 3 || (A[ind2d(i,j)] && (neigh == 2)));
                }
            #pragma omp barrier

            #pragma omp for collapse(2) private(neigh) schedule(static)
            for (int i = 1; i <= size; i++)
                for (int j = 1; j <= size; j++) {
                    neigh = B[ind2d(i-1,j-1)] + B[ind2d(i-1,j)] + B[ind2d(i-1,j+1)] +
                            B[ind2d(i,j-1)] + B[ind2d(i,j+1)] +
                            B[ind2d(i+1,j-1)] + B[ind2d(i+1,j)] + B[ind2d(i+1,j+1)];
                    A[ind2d(i,j)] = (neigh == 3 || (B[ind2d(i,j)] && (neigh == 2)));
                }
            #pragma omp barrier
        }
    }

    double t2 = wall_time();
    int ok = check_result(A, size);
    double t3 = wall_time();

    time_t ts1 = time(NULL);
    fmt_time_iso(ts1, r->end_time);
    r->duration_ms = (t3 - t0) * 1e3;
    strncpy(r->status, ok ? "OK" : "FAIL", sizeof(r->status));
    r->error_message[0] = '\0';

    free(A);
    free(B);
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (argc != 3) {
        if (rank == MASTER)
            fprintf(stderr, "Usage: %s <powmin> <powmax>\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    int powmin = atoi(argv[1]);
    int powmax = atoi(argv[2]);
    int nwork = powmax - powmin + 1;
    if (nprocs != nwork + 1) {
        if (rank == MASTER)
            fprintf(stderr, "Error: mpirun -np %d ./game_json %d %d\n",
                    nwork + 1, powmin, powmax);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank != MASTER) {
        result_t res;
        run_simulation(powmin + rank - 1, powmin, powmax, nprocs - 1, rank, &res);
        MPI_Send(&res, sizeof(res), MPI_BYTE, MASTER, TAG, MPI_COMM_WORLD);
    } else {
        FILE *f = fopen("results.json", "w");
        fprintf(f, "[\n");
        for (int src = 1; src <= nwork; src++) {
            result_t res;
            MPI_Recv(&res, sizeof(res), MPI_BYTE, src, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            fprintf(f,
                    "  {\n"
                    "    \"request_id\":      \"%s\",\n"
                    "    \"client_id\":       \"%s\",\n"
                    "    \"engine\":          \"%s\",\n"
                    "    \"powmin\":          %d,\n"
                    "    \"powmax\":          %d,\n"
                    "    \"start_time\":      \"%s\",\n"
                    "    \"end_time\":        \"%s\",\n"
                    "    \"duration_ms\":     %.3f,\n"
                    "    \"status\":          \"%s\",\n"
                    "    \"error_message\":   \"%s\",\n"
                    "    \"num_generations\": %d,\n"
                    "    \"board_size\":      %d,\n"
                    "    \"host_node\":       \"%s\",\n"
                    "    \"num_clients_active\": %d,\n"
                    "    \"timestamp\":       \"%s\"\n"
                    "  }%s\n",
                    res.request_id,
                    res.client_id,
                    res.engine,
                    res.powmin,
                    res.powmax,
                    res.start_time,
                    res.end_time,
                    res.duration_ms,
                    res.status,
                    res.error_message,
                    res.num_generations,
                    res.board_size,
                    res.host_node,
                    res.num_clients_active,
                    res.end_time,
                    (src < nwork ? "," : ""));
        }
        fprintf(f, "]\n");
        fclose(f);
        printf("Written results.json with %d entries\n", nwork);
    }

    MPI_Finalize();
    return 0;
}
