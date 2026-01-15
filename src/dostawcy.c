#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

#include "../include/dostawcy.h"
#include "../include/magazine.h"

static void *sig_handler(int sig_num);

static Delivery deliveries[DELIVERIES_COUNT];


int main(void) {
    struct sigaction sig_action;
    sig_action.sa_handler = (void *)sig_handler;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR1, &sig_action, NULL) != 0) {
        fprintf(stderr, "[Dostawcy][ERRN] Failed to initialize signal handling! (%s)\n", strerror(errno));
        return -1;
    }
    if (sigaction(SIGINT, &sig_action, NULL) != 0) {
        fprintf(stderr, "[Dostawcy][ERRN] Failed to initialize signal handling! (%s)\n", strerror(errno));
        return -1;
    }

    // Obtain IPC key
    key_t ipc_key = ftok(".", 69);
    if (ipc_key == -1) {
        fprintf(stderr, "[Dostawcy][ERRN] Unable to create IPC Key! (%s)\n", strerror(errno));
        return 1;
    }

    // Join the Message Queue for logging
    int msg_id = msgget(ipc_key, IPC_CREAT | 0600);
    if (msg_id == -1) {
        fprintf(stderr, "[Logging][ERRN] Unable to join the Message Queue! (%s)\n", strerror(errno));
        return 2;
    }

    // Join the Semaphore Set
    int sem_id = semget(ipc_key, 1, IPC_CREAT | 0600);
    if (sem_id == -1) {
        fprintf(stderr, "[Dostawcy][ERRN] Unable to join Semaphore Set! (%s)\n", strerror(errno));
        return 3;
    }

    // Join the Shared Memory segment
    int shm_id = shmget(ipc_key, sizeof(Magazine), IPC_CREAT | 0600);
    if (shm_id == -1) {
        fprintf(stderr, "[Dostawcy][ERRN] Unable to create Shared Memory segment! (%s)\n", strerror(errno));
        return 4;
    }

    // Attach to Shared Memory
    Magazine *magazine = (Magazine *)shmat(shm_id, 0, 0);
    if (magazine == (void *)-1) {
        fprintf(stderr, "[Dostawcy][ERRN] Failed to attach to Shared Memory segment! (%s)\n", strerror(errno));
        return 4;
    }

    // Fill information about delivieries
    deliveries[0].type = A;
    deliveries[1].type = B;
    deliveries[2].type = C;
    deliveries[3].type = D;
    for (int deliv = 0; deliv < DELIVERIES_COUNT; deliv++) {
        deliveries[deliv].magazine = magazine;
        deliveries[deliv].sem_id = sem_id;
        deliveries[deliv].msg_id = msg_id;
        deliveries[deliv].is_working = INIT_WORK;

        // Start a delivery
        if ((errno = pthread_create(&deliveries[deliv].tid, NULL, delivery_start, (void *)&deliveries[deliv])) != 0) {
            fprintf(stderr, "[Dostawcy][ERRN] Cannot start delivery! (%s)\n", strerror(errno));
            return 5;
        }
    }

    for (int i = 0; i < DELIVERIES_COUNT; i++) {
        if ((errno = pthread_join(deliveries[i].tid, NULL) != 0)) {
            fprintf(stderr, "[Dostawcy][ERRN] Failed to wait for thread termination! (%s)\n", strerror(errno));
            return 6;
        }
    }

    // Detach from SHM
    if (shmdt(magazine) == -1) {
        fprintf(stderr, "[Dostawcy][ERRN] Failed to deattach from Shared Memory segment! (%s)\n", strerror(errno));
        return 7;
    }

    return 0;
}

void *delivery_start(void *delivery_data) {
    Delivery *data = (Delivery *)delivery_data;

    int sem_id = data->sem_id;
    Magazine *magazine = data->magazine;

    while (true) {
        if (magazine->space_left <= 0)
            return (void *)0;

        if (data->is_working) {
            struct sembuf sem_op;
            sem_op.sem_num = 0;
            sem_op.sem_flg = 0;
            sem_op.sem_op = -1;

            if (semop(sem_id, &sem_op, 1) == -1) {
                fprintf(stderr, "[Dostawcy][ERRN] Failed to perform semaphore operation! (%s)\n", strerror(errno));
                return (void *)1;
            }

            switch (data->type) {
            case A: {
                if (magazine->space_left >= 1) {
                    int row = -1;
                    int col = -1;
                    find_free_slots(magazine, 1, &row, &col);

                    if (row >= 0 && col >= 0) {
                        magazine->magazine_racks[INDEX_2D(row, col, magazine)] = 'A';
                        magazine->space_left--;
                    } else
                        fprintf(stderr, "Something wrong with find_free_slots :(!\n");
                }
            }
            case B: {
                if (magazine->space_left >= 1) {
                    int row = -1;
                    int col = -1;
                    find_free_slots(magazine, 1, &row, &col);

                    if (row >= 0 && col >= 0) {
                        magazine->magazine_racks[INDEX_2D(row, col, magazine)] = 'B';
                        magazine->space_left--;
                    } else
                        fprintf(stderr, "Something wrong with find_free_slots :(!\n");
                }
            }
            case C: {
                if (magazine->space_left >= 2) {
                    int row = -1;
                    int col = -1;
                    find_free_slots(magazine, 2, &row, &col);

                    if (row >= 0 && col >= 0) {
                        int offset = 0;
                        for (int inserted = 0; inserted < 2; inserted++) {
                            if (col + offset >= magazine->columns) {
                                row++;
                                col = 0;
                                offset = 0;
                            }
                            magazine->magazine_racks[INDEX_2D(row, col + offset++, magazine)] = 'C';
                        }

                        magazine->space_left = magazine->space_left - 2;
                    }
                }
            }
            case D: {
                if (magazine->space_left >= 3) {
                    int row = -1;
                    int col = -1;
                    find_free_slots(magazine, 3, &row, &col);

                    if (row >= 0 && col >= 0) {
                        int offset = 0;
                        for (int inserted = 0; inserted < 3; inserted++) {
                            if (col + offset >= magazine->columns) {
                                row++;
                                col = 0;
                                offset = 0;
                            }
                            magazine->magazine_racks[INDEX_2D(row, col + offset++, magazine)] = 'D';
                        }

                        magazine->space_left = magazine->space_left - 3;
                    }
                }
            }
            }

            sem_op.sem_op = 1;
            if (semop(sem_id, &sem_op, 1) == -1) {
                fprintf(stderr, "[Dostawcy][ERRN] Failed to perform semaphore operation! (%s)\n", strerror(errno));
                return (void *)1;
            }
        }

        usleep(2000);
    }

    return (void *)0;
}


void *sig_handler(int sig_num) {
    if (sig_num == SIGUSR1)
        for (int i = 0; i < DELIVERIES_COUNT; i++)
            deliveries[i].is_working = !deliveries[i].is_working;
    else if (sig_num == SIGINT)
        exit(0);

    return NULL;
}
