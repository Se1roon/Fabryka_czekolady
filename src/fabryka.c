#include <sched.h>
#include <stdbool.h>

#include "../include/common.h"

static int msg_id = -1;
static Magazine *magazine;

static pid_t workers[2];
void worker(int type, int shm_id, int sem_id, int msg_id);

static void signal_handler_parent(int sig_num);
static void signal_handler_children(int sig_num);


int main() {
    if (signal(SIGINT, signal_handler_parent) == SIG_ERR) {
        fprintf(stderr, "%s[Factory] Failed to assign signal handler to SIGINT! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }
    if (signal(SIGUSR1, signal_handler_parent) == SIG_ERR) {
        fprintf(stderr, "%s[Factory] Failed to assign signal handler to SIGUSR1! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }
    if (signal(SIGUSR2, signal_handler_parent) == SIG_ERR) {
        fprintf(stderr, "%s[Factory] Failed to assign signal handler to SIGUSR2! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }

    key_t ipc_key = ftok(".", IPC_KEY_ID);
    if (ipc_key == -1) {
        fprintf(stderr, "%s[Factory] Failed to create key for IPC communication! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }

    int shm_id = shmget(ipc_key, sizeof(Magazine), IPC_CREAT | 0600);
    if (shm_id == -1) {
        fprintf(stderr, "%s[Factory] Failed to join the Shared Memory segment! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }

    int sem_id = semget(ipc_key, 1, IPC_CREAT | 0600);
    if (sem_id == -1) {
        fprintf(stderr, "%s[Factory] Failed to join the Semaphore Set! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }

    msg_id = msgget(ipc_key, IPC_CREAT | 0600);
    if (msg_id == -1) {
        fprintf(stderr, "%s[Factory] Failed to join the Message Queue! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }

    pid_t worker1 = fork();
    if (worker1 == -1) {
        send_log(msg_id, "%s[Factory] Failed to create Worker 1 process! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }
    if (worker1 == 0)
        worker(1, shm_id, sem_id, msg_id);

    pid_t worker2 = fork();
    if (worker2 == -1) {
        send_log(msg_id, "%s[Factory] Failed to create Worker 2 process! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }
    if (worker2 == 0)
        worker(2, shm_id, sem_id, msg_id);

    workers[0] = worker1;
    workers[1] = worker2;

    while (true) {
        for (int i = 0; i < 2; i++) {
            int status = -1;
            if (waitpid(workers[i], &status, WNOHANG) < 0) {
                send_log(msg_id, "%s[Factory] Failed to wait for worker termination! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);
            }

            if (WIFEXITED(status))
                send_log(msg_id, "%s[Factory] Worker %d has terminated (code %d)%s", INFO_CLR_SET, i, WEXITSTATUS(status), CLR_RST);
        }
    }

    return 0;
}


void worker(int type, int shm_id, int sem_id, int msg_id) {
    if (signal(SIGINT, signal_handler_children) == SIG_ERR) {
        send_log(msg_id, "%s[Factory: Worker %d] Failed to assign signal handler to SIGINT! (%s)%s\n", ERROR_CLR_SET, type, strerror(errno), CLR_RST);
        exit(-1);
    }

    magazine = (Magazine *)shmat(shm_id, NULL, 0);
    if (magazine == (void *)-1) {
        send_log(msg_id, "%s[Factory] Failed to attach to Shared Memory segment! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);
        exit(-1);
    }

    struct sembuf lock = {0, -1, 0};
    struct sembuf unlock = {0, 1, 0};

    send_log(msg_id, "%s[Worker] Worker %d started%s", INFO_CLR_SET, type, CLR_RST);

    while (1) {
        semop(sem_id, &lock, 1);

        bool found_A = false;
        bool found_B = false;

        // Look for A
        for (int i = IsA; i <= IkA; i++) {
            if (magazine->buffer[i] == 'A') {
                found_A = true;
                magazine->buffer[i] = 0;
                break;
            }
        }
        // Look for B
        for (int i = IsB; i <= IkB; i++) {
            if (magazine->buffer[i] == 'B') {
                found_B = true;
                magazine->buffer[i] = 0;
                break;
            }
        }

        // Type X of chocolate
        if (type == 1) {
            bool found_C = false;
            // Look for C
            for (int i = IsC; i <= IkC; i++) {
                if (magazine->buffer[i] == 'C') {
                    found_C = true;
                    magazine->buffer[i] = 0;
                    break;
                }
            }
            if (found_A && found_B && found_C) {
                magazine->type1_produced++;
                send_log(msg_id, "%s[Worker] PRODUCED chocolate type X!%s", INFO_CLR_SET, CLR_RST);
                if (magazine->type1_produced == X_TYPE_TO_PRODUCE) {
                    send_log(msg_id, "%s[Worker X] PRODUCTION X COMPLETE | Sending notification to Director%s", INFO_CLR_SET, CLR_RST);
                    kill(getppid(), SIGUSR1);
                    send_log(msg_id, "%s[Worker X] Killing myself%s", INFO_CLR_SET, CLR_RST);
                    semop(sem_id, &unlock, 1);
                    break;
                }
            } else
                send_log(msg_id, "%s[Worker] Not enough components for type X!%s", WARNING_CLR_SET, CLR_RST);
        } else if (type == 2) {
            bool found_D = false;
            // Look for D
            for (int i = IsD; i <= IkD; i++) {
                if (magazine->buffer[i] == 'D') {
                    found_D = true;
                    magazine->buffer[i] = 0;
                    break;
                }
            }

            if (found_A && found_B && found_D) {
                magazine->type2_produced++;
                send_log(msg_id, "%s[Worker] PRODUCED chocolate type Y!%s", INFO_CLR_SET, CLR_RST);
                if (magazine->type2_produced == Y_TYPE_TO_PRODUCE) {
                    send_log(msg_id, "%s[Worker Y] PRODUCTION Y COMPLETE | Sending notification to Director%s", INFO_CLR_SET, CLR_RST);
                    kill(getppid(), SIGUSR2);
                    send_log(msg_id, "%s[Worker Y] Killing myself%s", INFO_CLR_SET, CLR_RST);
                    semop(sem_id, &unlock, 1);
                    break;
                }
            } else
                send_log(msg_id, "%s[Worker] Not enough components for type Y!%s", WARNING_CLR_SET, CLR_RST);
        }

        semop(sem_id, &unlock, 1);

        sched_yield();
    }

    shmdt(magazine);
    exit(0);
}

void signal_handler_parent(int sig_num) {
    if (sig_num == SIGINT) {
        kill(workers[0], SIGINT);
        kill(workers[1], SIGINT);
    } else if (sig_num == SIGUSR1) {
        kill(getppid(), SIGUSR1);
    } else if (sig_num == SIGUSR2) {
        kill(getppid(), SIGUSR2);
    }
}

void signal_handler_children(int sig_num) {
    if (sig_num == SIGINT) {
        send_log(msg_id, "%s[Worker] Received SIGINT... Terminating%s", INFO_CLR_SET, CLR_RST);
        shmdt(magazine);
        exit(0);
    }
}
