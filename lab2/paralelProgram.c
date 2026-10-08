///usr/bin/cc -o /tmp/${0%%.c} -pthread $0 && exec /tmp/${0%%.c}
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <pthread.h>
#include <time.h>

typedef struct {
	size_t n;
    size_t k; // turn
	size_t current_turn;
    size_t start_score1;
    size_t start_score2;

    size_t start;
	size_t end;

	size_t wins1;
	size_t wins2;
	size_t draws;

	unsigned seed;
	size_t number;

	double elapsed_sec;
} ThreadArgs;

static int roll_two_dice(unsigned *seed) {
	int a = rand_r(seed) % 6 + 1;
	int b = rand_r(seed) % 6 + 1;
	return a + b;
}

static int run_experiment(int K, int current_turn,
						int start_score1, int start_score2,
                        unsigned *seed) {
    int sum1 = start_score1;
	int sum2 = start_score2;

    for (int t = current_turn; t <= K; ++t) {
        sum1 += roll_two_dice(seed);
        sum2 += roll_two_dice(seed);
    }

    if (sum1 > sum2) return 1;
    if (sum2 > sum1) return 2;
    return 0;
}

static void *work(void *_args)
{
    ThreadArgs *args = (ThreadArgs *)_args;

	struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    size_t w1 = 0, w2 = 0, d = 0;

    for (int i = args->start; i < args->end; ++i) {
        int w = run_experiment(args->k, args->current_turn,
                            args->start_score1, args->start_score2,
                            &args->seed);
        if (w == 1) w1++;
        else if (w == 2) w2++;
        else d++;
    }

    args->wins1 = w1;
    args->wins2 = w2;
    args->draws = d;

    printf("thread %zu done: experiments [%zu, %zu)\n",
        args->number, args->start, args->end);

	clock_gettime(CLOCK_MONOTONIC, &t1);
    args->elapsed_sec = (t1.tv_sec  - t0.tv_sec)
                   + (t1.tv_nsec - t0.tv_nsec) * 1e-9;

    return NULL;
}

int main(int argc, char **argv)
{
    size_t K            = (argc > 1) ? (size_t)atoi(argv[1]) : 10;
    size_t current_turn = (argc > 2) ? (size_t)atoi(argv[2]) : 1;
    size_t start_score1 = (argc > 3) ? (size_t)atoi(argv[3]) : 0;
    size_t start_score2 = (argc > 4) ? (size_t)atoi(argv[4]) : 0;
    size_t N            = (argc > 5) ? (size_t)atoi(argv[5]) : 1000000;
    size_t n_threads    = (argc > 6) ? (size_t)atoi(argv[6]) : sysconf(_SC_NPROCESSORS_ONLN - 1);

    if (current_turn < 1) current_turn = 1;
    if (current_turn > K) current_turn = K;
    if (N < 1) N = 1;

    pthread_t  *threads     = malloc(n_threads * sizeof(pthread_t));
    ThreadArgs *thread_args = malloc(n_threads * sizeof(ThreadArgs));
    if (!threads || !thread_args) { perror("malloc"); return 1; }

    size_t chunk = N / n_threads;
    size_t rem   = N % n_threads;

    size_t pos = 0;
    for (size_t t = 0; t < n_threads; ++t) {
        size_t len = chunk + (t < rem ? 1 : 0);

        thread_args[t] = (ThreadArgs){
            .n            = N,
            .k            = K,
            .current_turn = current_turn,
            .start_score1 = start_score1,
            .start_score2 = start_score2,
            .start        = pos,
            .end          = pos + len,
            .wins1        = 0,
            .wins2        = 0,
            .draws        = 0,
            .seed         = (unsigned int)time(NULL) ^ (unsigned int)getpid(),
            .number       = t,
        };
        pos += len;

        int rc = pthread_create(&threads[t], NULL, work, &thread_args[t]);
        if (rc != 0) { fprintf(stderr, "pthread_create: %d\n", rc); return 1; }
    }

    for (size_t t = 0; t < n_threads; ++t) {
        pthread_join(threads[t], NULL);
        printf("thread %zu: %.6f s\n", t, thread_args[t].elapsed_sec);
    }

    size_t w1 = 0, w2 = 0, d = 0;
    for (size_t t = 0; t < n_threads; ++t) {
        w1 += thread_args[t].wins1;
        w2 += thread_args[t].wins2;
        d  += thread_args[t].draws;
    }

    printf("K = %zu; turn = %zu; score = %zu : %zu; N = %zu;\n",
        K, current_turn, start_score1, start_score2, N);
    printf("player 1 wins: %zu (%.4f)\n", w1, (double)w1 / N);
    printf("player 2 wins: %zu (%.4f)\n", w2, (double)w2 / N);
    printf("draws:         %zu (%.4f)\n", d,  (double)d  / N);

    free(thread_args);
    free(threads);
    return 0;
}