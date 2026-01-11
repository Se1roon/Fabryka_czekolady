#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

#include "../include/common.h"
#include "../include/fabryka.h"
#include "../include/logging.h"

static void *sig_handler(int sig_num);

static bool stations_work = INIT_WORK;

static SHM_DATA *magazine = NULL;
static int msg_id = -1;


int main(void) {
    struct sigaction sig_action;
    sig_action.sa_handler = (void *)sig_handler;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR1, &sig_action, NULL) != 0) {
    }
    if (sigaction(SIGINT, &sig_action, NULL) != 0) {
    }

    // Obtain IPC key
    key_t ipc_key = ftok(".", 69);
    if (ipc_key == -1) {
        fprintf(stderr, "[Fabryka][ERRN] Unable to create IPC Key! (%s)\n", strerror(errno));
        return 1;
    }

    // Join the Semaphore Set
    int sem_id = semget(ipc_key, 1, IPC_CREAT | 0600);
    if (sem_id == -1) {
        fprintf(stderr, "[Fabryka][ERRN] Unable to join Semaphore Set! (%s)\n", strerror(errno));
        return 2;
    }

    // Join the Message Queue
    msg_id = msgget(ipc_key, IPC_CREAT | 0600);
    if (msg_id == -1) {
        fprintf(stderr, "[Logging][ERRN] Unable to join the Message Queue! (%s)\n", strerror(errno));
        return -1;
    }

    // Join the Shared Memory segment
    int shm_id = shmget(ipc_key, sizeof(SHM_DATA), IPC_CREAT | 0600);
    if (shm_id == -1) {
        fprintf(stderr, "[Fabryka][ERRN] Unable to create Shared Memory segment! (%s)\n", strerror(errno));
        return 3;
    }

    // Attach to Shared Memory
    magazine = (SHM_DATA *)shmat(shm_id, 0, 0);
    if (magazine == (void *)-1) {
        fprintf(stderr, "[Fabryka][ERRN] Failed to attach to Shared Memory segment! (%s)\n", strerror(errno));
        return 4;
    }

    pthread_t s1_pid; // TID of stanowisko_1
    pthread_t s2_pid; // TID of stanowisko_2

    station_t station_data;
    station_data.sem_id = sem_id;
    station_data.msg_id = msg_id;
    station_data.magazine_data = magazine;

    if ((errno = pthread_create(&s1_pid, NULL, station_1, (void *)&station_data)) != 0) {
        fprintf(stderr, "[Fabryka][ERRN] Failed to create Station 1! (%s)\n", strerror(errno));
        return 5;
    }
    if ((errno = pthread_create(&s2_pid, NULL, station_2, (void *)&station_data)) != 0) {
        fprintf(stderr, "[Fabryka][ERRN] Failed to create Station 2! (%s)\n", strerror(errno));
        return 5;
    }

    // Pointless??
    if ((errno = pthread_join(s1_pid, NULL)) != 0) {
        fprintf(stderr, "[Fabryka][ERRN] Failed to wait for Station 1 to finish! (%s)\n", strerror(errno));
        return 6;
    }
    if ((errno = pthread_join(s2_pid, NULL)) != 0) {
        fprintf(stderr, "[Fabryka][ERRN] Failed to wait for Station 2 to finish! (%s)\n", strerror(errno));
        return 6;
    }

    return 0;
}

void *station_1(void *station_data) {
    station_t *s_data = (station_t *)station_data;

    int sem_id = s_data->sem_id;
    int msg_id = s_data->msg_id;
    SHM_DATA *mag_data = s_data->magazine_data;

    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_flg = 0;

    bool waiting_for_components = false;
    while (true) {
        if (stations_work) {
            sem_op.sem_op = -1;
            if (semop(sem_id, &sem_op, 1) == -1) {
                fprintf(stderr, "[Fabryka][ERRN] Failed to perform semaphore operation! (%s)\n", strerror(errno));
                return (void *)1;
            }

            if (mag_data->a_count > 0 && mag_data->b_count > 0 && mag_data->c_count > 0) {
                mag_data->a_count--;
                mag_data->b_count--;
                mag_data->c_count--;
                mag_data->type1_produced++;

                waiting_for_components = false;

                if (mag_data->type1_produced % 200 == 0)
                    write_log(msg_id, "[Fabryka][INFO] Produced 200 type 1 chocolate!");
            } else if (!waiting_for_components) {
                write_log(msg_id, "[Fabryka][INFO] Waiting for type1 components!");
                waiting_for_components = true;
            }

            sem_op.sem_op = 1;
            if (semop(sem_id, &sem_op, 1) == -1) {
                fprintf(stderr, "[Fabryka][ERRN] Failed to perform semaphore operation! (%s)\n", strerror(errno));
                return (void *)1;
            }
        }

        usleep(3000);
    }

    return (void *)0;
}

void *station_2(void *station_data) {
    station_t *s_data = (station_t *)station_data;

    int sem_id = s_data->sem_id;
    SHM_DATA *mag_data = s_data->magazine_data;

    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_flg = 0;

    bool waiting_for_components = false;
    while (true) {
        if (stations_work) {
            sem_op.sem_op = -1;
            if (semop(sem_id, &sem_op, 1) == -1) {
                fprintf(stderr, "[Fabryka][ERRN] Failed to perform semaphore operation! (%s)\n", strerror(errno));
                return (void *)1;
            }

            if (mag_data->a_count && mag_data->b_count && mag_data->d_count) {
                mag_data->a_count--;
                mag_data->b_count--;
                mag_data->d_count--;
                mag_data->type2_produced++;

                waiting_for_components = false;

                if (mag_data->type2_produced % 200 == 0)
                    write_log(msg_id, "[Fabryka][INFO] Produced 200 type 2 chocolate!");
            } else if (!waiting_for_components) {
                write_log(msg_id, "[Fabryka][INFO] Waiting for type2 components!");
                waiting_for_components = true;
            }

            sem_op.sem_op = 1;
            if (semop(sem_id, &sem_op, 1) == -1) {
                fprintf(stderr, "[Fabryka][ERRN] Failed to perform semaphore operation! (%s)\n", strerror(errno));
                return (void *)1;
            }
        }

        usleep(3000);
    }

    return (void *)0;
}

void write_log(int msg_id, char *message) {
    LogMessage log_msg;
    log_msg.mtype = 2;
    strncpy(log_msg.message, message, LOG_MESSAGE_MAX_LEN - 1);
    log_msg.message[LOG_MESSAGE_MAX_LEN - 1] = '\0';

    msgsnd(msg_id, &log_msg, sizeof(log_msg.message), 0);
}

void *sig_handler(int sig_num) {
    if (sig_num == SIGUSR1) {
        stations_work = !stations_work;
    } else if (sig_num == SIGINT) {
        exit(0);
    }

    return NULL;
}
