#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "../include/common.h"
#include "../include/dostawcy.h"


int main(void) {
    // Obtain IPC key
    key_t ipc_key = ftok(".", 69);
    if (ipc_key == -1) {
        fprintf(stderr, "[Dostawcy][ERRN] Unable to create IPC Key! (%s)\n", strerror(errno));
        return 1;
    }

    // Join the Semaphore Set
    int sem_id = semget(ipc_key, 2, IPC_CREAT | 0600);
    if (sem_id == -1) {
        fprintf(stderr, "[Dostawcy][ERRN] Unable to join Semaphore Set! (%s)\n", strerror(errno));
        return 2;
    }

    // Join the Shared Memory segment
    int shm_id = shmget(ipc_key, sizeof(SHM_DATA), IPC_CREAT | 0600);
    if (shm_id == -1) {
        fprintf(stderr, "[Dostawcy][ERRN] Unable to create Shared Memory segment! (%s)\n", strerror(errno));
        return 3;
    }

    // Attach to Shared Memory
    SHM_DATA* sh_data = (SHM_DATA*)shmat(shm_id, 0, 0);
    if (sh_data == (void*)-1) {
        fprintf(stderr, "[Dostawcy][ERRN] Failed to attach to Shared Memory segment! (%s)\n", strerror(errno));
        return 4;
    }

    delivery_t delivery_guys[DELIVERY_GUYS_COUNT];
    delivery_guys[0].type = A;
    delivery_guys[1].type = B;
    delivery_guys[2].type = C;
    delivery_guys[3].type = D;

    for (int d_guy = 0; d_guy < DELIVERY_GUYS_COUNT; d_guy++) {
        delivery_guys[d_guy].sem_id = sem_id;
        delivery_guys[d_guy].magazine_data = sh_data;

        if (pthread_create(&delivery_guys[d_guy].tid, NULL, delivery, (void*)&delivery_guys[d_guy]) != 0) {
            fprintf(stderr, "[Dostawcy][ERRN] Cannot start delivery! (%s)\n", strerror(errno));
            return 5;
        }
    }

    for (int i = 0; i < DELIVERY_GUYS_COUNT; i++) {
        if (pthread_join(delivery_guys[i].tid, NULL) != 0) {
            fprintf(stderr, "[Dostawcy][ERRN] Failed to wait for thread termination! (%s)\n", strerror(errno));
            return 6;
        }
    }

    return 0;
}

void* delivery(void* delivery_data) {
    delivery_t* d_data = (delivery_t*)delivery_data;

    printf("Dostawca %d | %zu\n", d_data->type, d_data->magazine_data->capacity);

    return NULL;
}