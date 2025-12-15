#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int main(void) {
    // Obtain IPC key
    key_t ipc_key = ftok(".", 69);
    if (ipc_key == -1) {
        fprintf(stderr, "[Dyrektor][ERRN] Unable to create IPC Key! (%s)\n", strerror(errno));
        return 1;
    }

    // Create Semaphore Set
    int sem_id = semget(ipc_key, 2, IPC_CREAT | IPC_EXCL | 0600);
    if (sem_id == -1) {
        fprintf(stderr, "[Dyrektor][ERRN] Unable to create Semaphore Set! (%s)\n", strerror(errno));
        return 2;
    }

    // Initialize Semaphore Set to 1s
    unsigned short values[2] = {1, 1};
    union semun arg;
    arg.array = values;
    if (semctl(sem_id, 0, SETALL, arg) == -1) {
        fprintf(stderr, "[Dyrektor][ERRN] Unable to initialize Semaphore Set! (%s)\n", strerror(errno));
        clean_up(sem_id, -1);
        return 3;
    }

    // Create Shared Memory segment (initialized with 0s)
    int shm_id = shmget(ipc_key, sizeof(SHM_DATA), IPC_CREAT | IPC_EXCL | 0600);
    if (shm_id == -1) {
        fprintf(stderr, "[Dyrektor][ERRN] Unable to create Shared Memory segment! (%s)\n", strerror(errno));
        clean_up(sem_id, -1);
        return 4;
    }

    pid_t cpid;
    cpid = fork();
    switch (cpid) {
        case -1: fprintf(stderr, "[Dyrektor][main][ERRN] Failed to create child process!\n"); exit(1);
        case 0: // Child
            if (execl("./bin/fabryka", "./bin/fabryka", NULL) < 0) {
                fprintf(stderr, "[Dyrektor][main][ERRN] Faield to create Fabryka process!\n");
                exit(1);
            }
            break;
        default: // Parent
            printf("Parent\n");
    };


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