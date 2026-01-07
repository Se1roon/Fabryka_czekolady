#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../include/common.h"
#include "../include/dyrektor.h"

union semun {
    int value;
    struct semid_ds *buf;
    unsigned short *array;
};

static int sem_id;
static int shm_id;

int main(void) {
    struct sigaction sig_action;
    sig_action.sa_handler = (void *)sig_handler;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sig_action, NULL) != 0) {
        fprintf(stderr,
                "[Dyrektor][ERRN] Failed to initialize signal handling! (%s)\n",
                strerror(errno));
        return -1;
    }

    // Obtain IPC key
    key_t ipc_key = ftok(".", 69);
    if (ipc_key == -1) {
        fprintf(stderr, "[Dyrektor][ERRN] Unable to create IPC Key! (%s)\n",
                strerror(errno));
        return 1;
    }

    // Create Semaphore Set
    sem_id = semget(ipc_key, 2, IPC_CREAT | IPC_EXCL | 0600);
    if (sem_id == -1) {
        fprintf(stderr,
                "[Dyrektor][ERRN] Unable to create Semaphore Set! (%s)\n",
                strerror(errno));
        return 2;
    }

    // Initialize Semaphore Set to 1s (sem 0 -> magazine sync | sem 1 -> log
    // file sync)
    unsigned short values[2] = {1, 1};
    union semun arg;
    arg.array = values;
    if (semctl(sem_id, 0, SETALL, arg) == -1) {
        fprintf(stderr,
                "[Dyrektor][ERRN] Unable to initialize Semaphore Set! (%s)\n",
                strerror(errno));
        clean_up(sem_id, -1);
        return 3;
    }

    // Create Shared Memory segment (initialized with 0s)
    shm_id = shmget(ipc_key, sizeof(SHM_DATA), IPC_CREAT | IPC_EXCL | 0600);
    if (shm_id == -1) {
        fprintf(
            stderr,
            "[Dyrektor][ERRN] Unable to create Shared Memory segment! (%s)\n",
            strerror(errno));
        clean_up(sem_id, -1);
        return 4;
    }
    // Initialize capacity in SHM to MAG_CAPACITY
    SHM_DATA *shm = (SHM_DATA *)shmat(shm_id, 0, 0);
    if (shm == (void *)-1) {
        fprintf(stderr,
                "[Dyrektor][ERRN] Failed to attach to Shared Memory segment! "
                "(%s)\n",
                strerror(errno));
        clean_up(sem_id, shm_id);
        return 5;
    }
    *(size_t *)shm = (size_t)MAG_CAPACITY;
    /*
    if (shmdt(shm) == -1) {
        fprintf(stderr, "[Dyrektor][ERRN] Failed to deattach from Shared memory
    segment! (%s)\n", strerror(errno)); clean_up(sem_id, shm_id); return 6;
    }
        */

    pid_t child_processes[2]; // Dostawcy, Fabryka

    UI_data ui_data;
    ui_data.children = child_processes;
    ui_data.data = shm;
    pthread_t uint_tid; // User interface TID
    if ((errno = pthread_create(&uint_tid, NULL, handle_user_interface,
                                (void *)&ui_data)) != 0) {
        fprintf(stderr, "[Dyrektor][ERRN] Failed to create Thread! (%s)\n",
                strerror(errno));
        return 9;
    }

    // Start child processes
    for (int cproc = 0; cproc < 2; cproc++) {
        pid_t cpid = fork();
        switch (cpid) {
        case -1: {
            fprintf(
                stderr,
                "[Dyrektor][main][ERRN] Failed to create child process! (%s)\n",
                strerror(errno));
            clean_up(sem_id, shm_id);
            return 7;
        }
        case 0: {
            const char *exec_prog =
                (cproc == 0) ? "./bin/dostawcy" : "./bin/fabryka";
            execl(exec_prog, exec_prog, NULL);

            // execl returns = error
            fprintf(stderr,
                    "[Dyrektor][ERRN] Failed to execute %s program! (%s)\n",
                    exec_prog, strerror(errno));
            clean_up(sem_id, shm_id);
            return 8;
        }
        default:
            child_processes[cproc] = cpid;
        };
    }

    // Wait for child processes to finish
    for (int i = 0; i < 2; i++) {
        pid_t child_pid = child_processes[i];

        int status = -1;
        if (waitpid(child_pid, &status, 0) < 0) {
            fprintf(
                stderr,
                "[Dyrektor][ERRN] Failed to wait for child termination! (%s)\n",
                strerror(errno));
            clean_up(sem_id, shm_id);
        }

        if (WIFEXITED(status))
            printf("[Dyrektor] Child process %d has terminated (code %d)\n",
                   child_pid, WEXITSTATUS(status));
    }

    if ((errno = pthread_join(uint_tid, NULL)) != 0) {
        fprintf(stderr,
                "[Dyrektor][ERRN] Failed to wait for User Interface Thread to "
                "finish! (%s)\n",
                strerror(errno));
        return 12;
    }

    clean_up(sem_id, shm_id);
    return 0;
}

void clean_up(int sem_id, int shm_id) {
    if (shm_id > 0 && shmctl(shm_id, IPC_RMID, NULL) == -1) {
        fprintf(stderr, "Failed to clean up Shared Memory segment! (%s)\n",
                strerror(errno));
        exit(10);
    }
    if (sem_id > 0 && semctl(sem_id, 0, IPC_RMID) == -1) {
        fprintf(stderr, "Failed to clean up Semaphore Set! (%s)\n",
                strerror(errno));
        exit(11);
    }

    return;
}

void *handle_user_interface(void *ui_data) {
    UI_data *u_data = (UI_data *)ui_data;

    bool f_work = true;
    bool d_work = true;

    char *command = NULL;
    size_t command_len = -1;

    printf("Type help to see the list of available commands!\n");
    while (true) {
        printf("\n> ");
        if (getline(&command, &command_len, stdin) == -1) {
            fprintf(stderr, "Unable to read user's command\n");
        } else {
            command[command_len-- - 1] = 0; // Remove the trailing newline

            if (strncmp(command, "quit", 4) == 0) {
                kill(u_data->children[0], SIGINT);
                kill(u_data->children[1], SIGINT);
                break;
            } else if (strncmp(command, "stats", 5) == 0) {
                struct sembuf sem_op;
                sem_op.sem_num = 0;
                sem_op.sem_flg = 0;
                sem_op.sem_op = -1;
                semop(sem_id, &sem_op, 1);

                printf("Fabryka is %s\n", f_work ? "ON" : "OFF");
                printf("Dostawcy is %s\n\n", d_work ? "ON" : "OFF");

                printf("Type 1 chocolate produced: %zu\n",
                       u_data->data->type1_produced);
                printf("Type 2 chocolate produced: %zu\n",
                       u_data->data->type2_produced);

                printf("\nMagazine state:\n");
                printf("A = %zu\n", u_data->data->a_count);
                printf("B = %zu\n", u_data->data->b_count);
                printf("C = %zu\n", u_data->data->c_count);
                printf("D = %zu\n", u_data->data->d_count);

                sem_op.sem_op = 1;
                semop(sem_id, &sem_op, 1);
            } else if (strncmp(command, "td", 2) == 0) {
                // Sent SIGUSR1 to Dostawcy
                kill(u_data->children[0], SIGUSR1);
                d_work = !d_work;
                printf("Dostawcy switched to %s\n", d_work ? "ON" : "OFF");
            } else if (strncmp(command, "tf", 2) == 0) {
                // Sent SIGUSR1 to Fabryka
                kill(u_data->children[1], SIGUSR1);
                f_work = !f_work;
                printf("Fabryka switched to %s\n", f_work ? "ON" : "OFF");
            } else if (strncmp(command, "help", 4) == 0) {
                printf("\nstats - Shows the state of factory and deliveries as "
                       "well as the number of chocolate "
                       "produced.\n");
                printf("td    - Toggles the deliveries\n");
                printf("tf    - Toggles the factory\n");
                printf("quit  - Quits\n\n");
            }
        }
    }

    free(command);
    return NULL;
}

void *sig_handler(int sig_num) {
    if (sig_num == SIGINT) {
        write(1, "[Dyrektor][SIGNAL] Received SIGINT!\n", 38);
        clean_up(sem_id, shm_id);
        exit(0);
    }

    return NULL;
}
