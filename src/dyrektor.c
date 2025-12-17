#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../include/common.h"

union semun {
    int value;
    struct semid_ds* buf;
    unsigned short* array;
};

void clean_up(int sem_id, int shm_id);
void* sig_handler(int sig_num);

int sem_id;
int shm_id;

int main(void) {
    struct sigaction sig_action;
    sig_action.sa_handler = (void*)sig_handler;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sig_action, NULL) != 0) {
        fprintf(stderr, "[Dyrektor][ERRN] Failed to initialize signal handling! (%s)\n", strerror(errno));
        return -1;
    }


    // Obtain IPC key
    key_t ipc_key = ftok(".", 69);
    if (ipc_key == -1) {
        fprintf(stderr, "[Dyrektor][ERRN] Unable to create IPC Key! (%s)\n", strerror(errno));
        return 1;
    }

    // Create Semaphore Set
    sem_id = semget(ipc_key, 2, IPC_CREAT | IPC_EXCL | 0600);
    if (sem_id == -1) {
        fprintf(stderr, "[Dyrektor][ERRN] Unable to create Semaphore Set! (%s)\n", strerror(errno));
        return 2;
    }

    // Initialize Semaphore Set to 1s (sem 0 -> magazine sync | sem 1 -> log file sync)
    unsigned short values[2] = {1, 1};
    union semun arg;
    arg.array = values;
    if (semctl(sem_id, 0, SETALL, arg) == -1) {
        fprintf(stderr, "[Dyrektor][ERRN] Unable to initialize Semaphore Set! (%s)\n", strerror(errno));
        clean_up(sem_id, -1);
        return 3;
    }

    // Create Shared Memory segment (initialized with 0s)
    shm_id = shmget(ipc_key, sizeof(SHM_DATA), IPC_CREAT | IPC_EXCL | 0600);
    if (shm_id == -1) {
        fprintf(stderr, "[Dyrektor][ERRN] Unable to create Shared Memory segment! (%s)\n", strerror(errno));
        clean_up(sem_id, -1);
        return 4;
    }
    // Initialize capacity in SHM to MAG_CAPACITY
    SHM_DATA* shm = (SHM_DATA*)shmat(shm_id, 0, 0);
    if (shm == (void*)-1) {
        fprintf(stderr, "[Dyrektor][ERRN] Failed to attach to Shared Memory segment! (%s)\n", strerror(errno));
        clean_up(sem_id, shm_id);
        return 5;
    }
    *(size_t*)shm = (size_t)MAG_CAPACITY;
    if (shmdt(shm) == -1) {
        fprintf(stderr, "[Dyrektor][ERRN] Failed to deattach from Shared memory segment! (%s)\n", strerror(errno));
        clean_up(sem_id, shm_id);
        return 6;
    }

    pid_t child_processes[2]; // Dostawcy, Fabryka

    // Start child processes
    for (int cproc = 0; cproc < 2; cproc++) {
        pid_t cpid = fork();
        switch (cpid) {
            case -1: {
                fprintf(stderr, "[Dyrektor][main][ERRN] Failed to create child process! (%s)\n", strerror(errno));
                clean_up(sem_id, shm_id);
                return 7;
            }
            case 0: {
                const char* exec_prog = (cproc == 0) ? "./bin/dostawcy" : "./bin/fabryka";
                execl(exec_prog, exec_prog, NULL);

                // execl returns = error
                fprintf(stderr, "[Dyrektor][ERRN] Failed to execute %s program! (%s)\n", exec_prog, strerror(errno));
                clean_up(sem_id, shm_id);
                return 8;
            }
            default: child_processes[cproc] = cpid;
        };
    }

    // Wait for child processes to finish
    for (int i = 0; i < 2; i++) {
        pid_t child_pid = child_processes[i];

        int status = -1;
        if (waitpid(child_pid, &status, 0) < 0) {
            fprintf(stderr, "[Dyrektor][ERRN] Failed to wait for child termination! (%s)\n", strerror(errno));
            clean_up(sem_id, shm_id);
        }

        if (WIFEXITED(status))
            printf("[Dyrektor] Child process %d has terminated (code %d)\n", child_pid, WEXITSTATUS(status));
    }

    clean_up(sem_id, shm_id);
    return 0;
}

void clean_up(int sem_id, int shm_id) {
    if (shm_id > 0 && shmctl(shm_id, IPC_RMID, NULL) == -1) {
        fprintf(stderr, "Failed to clean up Shared Memory segment! (%s)\n", strerror(errno));
        exit(10);
    }
    if (sem_id > 0 && semctl(sem_id, 0, IPC_RMID) == -1) {
        fprintf(stderr, "Failed to clean up Semaphore Set! (%s)\n", strerror(errno));
        exit(11);
    }

    return;
}

void* sig_handler(int sig_num) {
    if (sig_num == SIGINT) {
        write(1, "[Dyrektor][SIGNAL] Received SIGINT!\n", 38);
        clean_up(sem_id, shm_id);
        exit(0);
    }

    return NULL;
}