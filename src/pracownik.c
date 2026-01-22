#include <sched.h>
#include <stdbool.h>

#include "../include/common.h"

static int msg_id = -1;
static Magazine *magazine;

static bool is_active = true;

static void signal_handler(int sig_num, siginfo_t *sig_info, void *data);


int main(int argc, char *argv[]) {
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        fprintf(stderr, "%s[Worker] Failed to assign signal handler to SIGINT! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        fprintf(stderr, "%s[Worker] Failed to assign signal handler to SIGINT! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }


    if (argc != 2) {
        fprintf(stderr, "%s[Worker] Invalid arguments count!%s\n", ERROR_CLR_SET, CLR_RST);
        return -1;
    }

    char worker_type = argv[1][0];

    key_t ipc_key = ftok(".", IPC_KEY_ID);
    if (ipc_key == -1) {
        fprintf(stderr, "%s[Worker: %c] Failed to create key for IPC communication! (%s)%s\n", ERROR_CLR_SET, worker_type, strerror(errno), CLR_RST);
        return -1;
    }

    int shm_id = shmget(ipc_key, sizeof(Magazine), IPC_CREAT | 0600);
    if (shm_id == -1) {
        fprintf(stderr, "%s[Worker: %c] Failed to join the Shared Memory segment! (%s)%s\n", ERROR_CLR_SET, worker_type, strerror(errno), CLR_RST);
        return -1;
    }

    int sem_id = semget(ipc_key, 1, IPC_CREAT | 0600);
    if (sem_id == -1) {
        fprintf(stderr, "%s[Worker: %c] Failed to join the Semaphore Set! (%s)%s\n", ERROR_CLR_SET, worker_type, strerror(errno), CLR_RST);
        return -1;
    }

    msg_id = msgget(ipc_key, IPC_CREAT | 0600);
    if (msg_id == -1) {
        fprintf(stderr, "%s[Worker: %c] Failed to join the Message Queue! (%s)%s\n", ERROR_CLR_SET, worker_type, strerror(errno), CLR_RST);
        return -1;
    }

    magazine = shmat(shm_id, NULL, 0);
    if (magazine == (void *)-1) {
        fprintf(stderr, "%s[Worker: %c] Failed to attach to Shared Memory segment! (%s)%s\n", ERROR_CLR_SET, worker_type, strerror(errno), CLR_RST);
        return -1;
    }

    struct sembuf lock = {0, -1, 0};
    struct sembuf unlock = {0, 1, 0};

    send_log(msg_id, "%s[Worker: %c] Worker %c started%s", INFO_CLR_SET, worker_type, worker_type, CLR_RST);

    while (is_active) {
        while (semop(sem_id, &lock, 1) == -1) {
            if (errno != EINTR) {
                send_log(msg_id, "%s[Worker] Unable to perform semaphore WAIT operation! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);
                goto end_of_loop;
            }
        }

        int idx_A = -1;
        int idx_B = -1;

        // Look for A
        for (int i = IsA; i <= IkA; i++) {
            if (magazine->buffer[i] == 'A') {
                idx_A = i;
                break;
            }
        }
        // Look for B
        for (int i = IsB; i <= IkB; i++) {
            if (magazine->buffer[i] == 'B') {
                idx_B = i;
                break;
            }
        }

        // Type X of chocolate
        if (worker_type == 'X') {
            int idx_C = -1;
            // Look for C
            for (int i = IsC; i <= IkC; i++) {
                if (magazine->buffer[i] == 'C') {
                    idx_C = i;
                    break;
                }
            }
            if (idx_A != -1 && idx_B != -1 && idx_C != -1) {
                magazine->buffer[idx_A] = 0;
                magazine->buffer[idx_B] = 0;
                magazine->buffer[idx_C] = 0;
                magazine->typeX_produced++;
                send_log(msg_id, "%s[Worker: X] PRODUCED chocolate type X!%s", INFO_CLR_SET, CLR_RST);
                if (magazine->typeX_produced >= X_TYPE_TO_PRODUCE) {
                    send_log(msg_id, "%s[Worker: X] PRODUCTION X COMPLETE | Sending notification to Director%s", INFO_CLR_SET, CLR_RST);
                    kill(getppid(), SIGUSR1);
                    send_log(msg_id, "%s[Worker: X] Killing myself%s", INFO_CLR_SET, CLR_RST);

                    while (semop(sem_id, &unlock, 1) == -1) {
                        if (errno != EINTR) {
                            send_log(msg_id, "%s[Worker: %c] Unable to perform semaphore RELEASE operation! (%s)%s",
                                     ERROR_CLR_SET, worker_type, strerror(errno), CLR_RST);
                            break;
                        }
                    }
                    break;
                }
            } else
                send_log(msg_id, "%s[Worker: X] Not enough components for type X!%s", WARNING_CLR_SET, CLR_RST);
        } else if (worker_type == 'Y') {
            int idx_D = -1;
            // Look for D
            for (int i = IsD; i <= IkD; i++) {
                if (magazine->buffer[i] == 'D') {
                    idx_D = i;
                    break;
                }
            }

            if (idx_A != -1 && idx_B != -1 && idx_D != -1) {
                magazine->buffer[idx_A] = 0;
                magazine->buffer[idx_B] = 0;
                magazine->buffer[idx_D] = 0;
                magazine->typeY_produced++;
                send_log(msg_id, "%s[Worker: Y] PRODUCED chocolate type Y!%s", INFO_CLR_SET, CLR_RST);
                if (magazine->typeY_produced >= Y_TYPE_TO_PRODUCE) {
                    send_log(msg_id, "%s[Worker: Y] PRODUCTION Y COMPLETE | Sending notification to Director%s", INFO_CLR_SET, CLR_RST);
                    kill(getppid(), SIGUSR1);
                    send_log(msg_id, "%s[Worker: Y] Killing myself%s", INFO_CLR_SET, CLR_RST);

                    while (semop(sem_id, &unlock, 1) == -1) {
                        if (errno != EINTR) {
                            send_log(msg_id, "%s[Worker: %c] Unable to perform semaphore RELEASE operation! (%s)%s",
                                     ERROR_CLR_SET, worker_type, strerror(errno), CLR_RST);
                            break;
                        }
                    }
                    break;
                }
            } else
                send_log(msg_id, "%s[Worker: Y] Not enough components for type Y!%s", WARNING_CLR_SET, CLR_RST);
        }

        while (semop(sem_id, &unlock, 1) == -1) {
            if (errno != EINTR) {
                send_log(msg_id, "%s[Worker: %c] Unable to perform semaphore RELEASE operation! (%s)%s", ERROR_CLR_SET, worker_type, strerror(errno), CLR_RST);
                goto end_of_loop;
            }
        }

        sched_yield();
    }

end_of_loop:

    shmdt(magazine);
    return 0;
}

void signal_handler(int sig_num, siginfo_t *sig_info, void *data) {
    if (sig_num == SIGINT) {
        send_log(msg_id, "%s[Worker] Received SIGINT - Terminating%s", INFO_CLR_SET, CLR_RST);
        is_active = false;
    } else if (sig_num == SIGTERM) {
        if (getppid() != sig_info->si_pid) {
            send_log(msg_id, "%s[Worker] Something weird is happening... Notifying Director%s", ERROR_CLR_SET, CLR_RST);
            kill(getppid(), SIGUSR2);
        }
    }
}
