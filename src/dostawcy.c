#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "../include/common.h"
#include "../include/dostawcy.h"

bool d_guys_work = true;

int main(void) {
    struct sigaction sig_action;
    sig_action.sa_handler = (void*)sig_handler;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR1, &sig_action, NULL) != 0) {
        fprintf(stderr, "[Dostawcy][ERRN] Failed to initialize signal handling! (%s)\n", strerror(errno));
        return -1;
    }

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

    while (true) {
        if (d_guys_work) {
            for (int d_guy = 0; d_guy < DELIVERY_GUYS_COUNT; d_guy++) {
                delivery_guys[d_guy].sem_id = sem_id;
                delivery_guys[d_guy].magazine_data = sh_data;

                if ((errno = pthread_create(&delivery_guys[d_guy].tid, NULL, delivery, (void*)&delivery_guys[d_guy])) !=
                    0) {
                    fprintf(stderr, "[Dostawcy][ERRN] Cannot start delivery! (%s)\n", strerror(errno));
                    return 5;
                }
            }

            for (int i = 0; i < DELIVERY_GUYS_COUNT; i++) {
                int* rval = NULL;
                if ((errno = pthread_join(delivery_guys[i].tid, (void*)&rval)) != 0) {
                    fprintf(stderr, "[Dostawcy][ERRN] Failed to wait for thread termination! (%s)\n", strerror(errno));
                    return 6;
                }

                // Remember to check rval for -1 and detach
                // Now it is not needed because the flow will fall into it nevertheless
            }
        }
    }


    // Detach from SHM
    if (shmdt(sh_data) == -1) {
        fprintf(stderr, "[Dostawcy][ERRN] Failed to deattach from Shared Memory segment! (%s)\n", strerror(errno));
        return 7;
    }

    return 0;
}

void* delivery(void* delivery_data) {
    delivery_t* d_data = (delivery_t*)delivery_data;

    int sem_id = d_data->sem_id;
    SHM_DATA* mag_data = d_data->magazine_data;

    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_flg = 0;
    sem_op.sem_op = -1;

    if (semop(sem_id, &sem_op, 1) == -1) {
        fprintf(stderr, "[Dostawcy][ERRN] Failed to perform semaphore operation! (%s)\n", strerror(errno));
        return (void*)1;
    }

    /*
    printf(
        "Dostawca %d | A_count = %zu | B_count = %zu | C_count = %zu | D_count = %zu\n",
        d_data->type,
        mag_data->a_count,
        mag_data->b_count,
        mag_data->c_count,
        mag_data->d_count);
        */
    size_t magazine_count = get_magazine_count(mag_data);

    switch (d_data->type) {
        case A: {
            if (mag_data->capacity - magazine_count >= 1)
                mag_data->a_count = mag_data->a_count + 2;
            break;
        }
        case B: {
            if (mag_data->capacity - magazine_count >= 1)
                mag_data->b_count = mag_data->b_count + 2;
            break;
        }
        case C: {
            if (mag_data->capacity - magazine_count >= 2)
                mag_data->c_count++;
            break;
        }
        case D: {
            if (mag_data->capacity - magazine_count >= 3)
                mag_data->d_count++;
            break;
        }
    }

    sem_op.sem_op = 1;
    if (semop(sem_id, &sem_op, 1) == -1) {
        fprintf(stderr, "[Dostawcy][ERRN] Failed to perform semaphore operation! (%s)\n", strerror(errno));
        return (void*)1;
    }

    return (void*)0;
}

size_t get_magazine_count(SHM_DATA* magazine) {
    return magazine->a_count + magazine->b_count + 2 * magazine->c_count + 2 * magazine->d_count;
}

void* sig_handler(int sig_num) {
    if (sig_num == SIGUSR1) {
        write(1, "[Dostawcy][SIGNAL] Received toggle_delivery signal!\n", 54);
        d_guys_work = !d_guys_work;
    }

    return NULL;
}