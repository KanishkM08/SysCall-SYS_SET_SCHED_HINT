#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sched.h>

#define SYS_SET_SCHED_HINT 452
#define HINT_NONE 0
#define HINT_CPU_BOUND 1
#define HINT_LATENCY_SENSITIVE 3

/* A heavy workload to keep the CPU busy */
void do_work() {
    /* 'volatile' prevents the compiler from optimizing this loop away */
    volatile unsigned long long i;
    for (i = 0; i < 3000000000ULL; i++) {
        /* Busy loop to burn CPU cycles */
    }
}

void run_child_test(int hint, const char* hint_name) {
    struct timeval start, end;
    
    /* 1. Pin this process to CPU Core 0 to force contention */
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask) < 0) {
        perror("Failed to pin to CPU");
        exit(1);
    }

    /* 2. Apply the scheduling hint */
    if (syscall(SYS_SET_SCHED_HINT, getpid(), hint) != 0) {
        perror("System call failed");
        exit(1);
    }

    /* 3. Record start time, do heavy work, record end time */
    gettimeofday(&start, NULL);
    do_work();
    gettimeofday(&end, NULL);

    /* 4. Calculate elapsed time */
    double elapsed = (end.tv_sec - start.tv_sec) + 
                     (end.tv_usec - start.tv_usec) / 1000000.0;

    printf("[PID %d] %-25s finished in %.4f seconds\n", getpid(), hint_name, elapsed);
    exit(0);
}

int main() {
    printf("Starting CFS Scheduler Hint Benchmark...\n");
    printf("Pinning all tasks to CPU 0 to force contention.\n\n");

    /* Fork three processes with different hints */
    pid_t pid1 = fork();
    if (pid1 == 0) run_child_test(HINT_LATENCY_SENSITIVE, "LATENCY_SENSITIVE (Boost)");

    pid_t pid2 = fork();
    if (pid2 == 0) run_child_test(HINT_NONE, "NONE (Default)");

    pid_t pid3 = fork();
    if (pid3 == 0) run_child_test(HINT_CPU_BOUND, "CPU_BOUND (Penalized)");

    /* Wait for all children to finish */
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);

    printf("\nBenchmark complete.\n");
    return 0;
}
