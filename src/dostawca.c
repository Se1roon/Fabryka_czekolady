#include <sched.h>
#include <stdbool.h>

#include "../include/common.h"

static int msg_id = -1;
static Magazine *magazine = NULL;

static bool is_active = true;

void get_component_indexes(char component_type, int *start_out, int *end_out);
int find_consecutive(char *array, int start_i, int end_i, char search_term, int count);

int find_A();
int find_B();
int find_C();
int find_D();

static void signal_handler(int sig_num, siginfo_t *sig_info, void *data);


int main(int argc, char *argv[]) {
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        fprintf(stderr, "%s[Supplier] Failed to assign signal handler to SIGINT! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        fprintf(stderr, "%s[Supplier] Failed to assign signal handler to SIGINT! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }


    if (argc != 2) {
        fprintf(stderr, "%s[Supplier] Invalid arguments count!%s\n", ERROR_CLR_SET, CLR_RST);
        return -1;
    }

    char component = argv[1][0];

    key_t ipc_key = ftok(".", IPC_KEY_ID);
    if (ipc_key == -1) {
        fprintf(stderr, "%s[Supplier: %c] Failed to create key for IPC communication! (%s)%s\n", ERROR_CLR_SET, component, strerror(errno), CLR_RST);
        return -1;
    }

    int shm_id = shmget(ipc_key, sizeof(Magazine), IPC_CREAT | 0600);
    if (shm_id == -1) {
        fprintf(stderr, "%s[Supplier: %c] Failed to join the Shared Memory segment! (%s)%s\n", ERROR_CLR_SET, component, strerror(errno), CLR_RST);
        return -1;
    }

    int sem_id = semget(ipc_key, 9, IPC_CREAT | 0600);
    if (sem_id == -1) {
        fprintf(stderr, "%s[Supplier: %c] Failed to join the Semaphore Set! (%s)%s\n", ERROR_CLR_SET, component, strerror(errno), CLR_RST);
        return -1;
    }

    msg_id = msgget(ipc_key, IPC_CREAT | 0600);
    if (msg_id == -1) {
        fprintf(stderr, "%s[Supplier: %c] Failed to join the Message Queue! (%s)%s\n", ERROR_CLR_SET, component, strerror(errno), CLR_RST);
        return -1;
    }

    magazine = (Magazine *)shmat(shm_id, NULL, 0);
    if (magazine == (void *)-1) {
        fprintf(stderr, "%s[Supplier: %c] Failed to attach to Shared Memory segment! (%s)%s\n", ERROR_CLR_SET, component, strerror(errno), CLR_RST);
        return -1;
    }

    int start_index;
    int end_index;
    get_component_indexes(component, &start_index, &end_index);

    int sem_empty = get_semaphore_id(component, true);
    int sem_full = get_semaphore_id(component, false);

    struct sembuf sem_op_in[2];
    sem_op_in[0].sem_num = sem_empty;
    sem_op_in[0].sem_flg = 0;
    sem_op_in[0].sem_op = (component == 'A' || component == 'B') ? -1 : (component == 'C') ? -2
                                                                                           : -3;
    sem_op_in[1].sem_num = SEM_MAGAZINE;
    sem_op_in[1].sem_flg = SEM_UNDO;
    sem_op_in[1].sem_op = -1;
    struct sembuf sem_op_out[2];
    sem_op_out[0].sem_num = SEM_MAGAZINE;
    sem_op_out[0].sem_flg = SEM_UNDO;
    sem_op_out[0].sem_op = 1;
    sem_op_out[1].sem_num = sem_full;
    sem_op_out[1].sem_flg = 0;
    sem_op_out[1].sem_op = 1;

    send_log(msg_id, "%s[Supplier: %c] Starting deliveries of %c!%s", INFO_CLR_SET, component, component, CLR_RST);

    bool cap_reached = false;
    while (is_active) {
        bool lock_acquired = true;

        while (semop(sem_id, sem_op_in, 2) == -1) {
            if (errno != EINTR) {
                send_log(msg_id, "%s[Supplier: %c] Unable to perform semaphore WAIT operation! (%s)%s", ERROR_CLR_SET, component, strerror(errno), CLR_RST);
                exit(-1);
            }
            if (!is_active) {
                lock_acquired = false;
                break;
            }
        }

        if (!lock_acquired)
            break;

        if (component == 'A') {
            int s_i = find_A();
            if (s_i < 0 && !cap_reached) {
                send_log(msg_id, "%s[Supplier: %c] Not enough space for component %c!%s", WARNING_CLR_SET, component, component, CLR_RST);
                cap_reached = true;
            } else {
                sigset_t block_mask;
                sigemptyset(&block_mask);
                sigaddset(&block_mask, SIGINT);
                sigprocmask(SIG_BLOCK, &block_mask, NULL);

                magazine->buffer[s_i] = component;
                send_log(msg_id, "%s[Supplier: %c] Delivered %c component!%s", INFO_CLR_SET, component, component, CLR_RST);

                sigprocmask(SIG_UNBLOCK, &block_mask, NULL);

                cap_reached = false;
            }
        } else if (component == 'B') {
            int s_i = find_B();
            if (s_i < 0 && !cap_reached) {
                send_log(msg_id, "%s[Supplier: %c] Not enough space for component %c!%s", WARNING_CLR_SET, component, component, CLR_RST);
                cap_reached = true;
            } else {
                sigset_t block_mask;
                sigemptyset(&block_mask);
                sigaddset(&block_mask, SIGINT);
                sigprocmask(SIG_BLOCK, &block_mask, NULL);

                magazine->buffer[s_i] = component;
                send_log(msg_id, "%s[Supplier: %c] Delivered %c component!%s", INFO_CLR_SET, component, component, CLR_RST);

                sigprocmask(SIG_UNBLOCK, &block_mask, NULL);

                cap_reached = false;
            }
        } else if (component == 'C') {
            int s_i = find_C();
            if (s_i < 0 && !cap_reached) {
                send_log(msg_id, "%s[Supplier: %c] Not enough space for component %c!%s", WARNING_CLR_SET, component, component, CLR_RST);
                cap_reached = true;
            } else {
                sigset_t block_mask;
                sigemptyset(&block_mask);
                sigaddset(&block_mask, SIGINT);
                sigprocmask(SIG_BLOCK, &block_mask, NULL);

                magazine->buffer[s_i] = component;
                magazine->buffer[s_i + 1] = component;
                send_log(msg_id, "%s[Supplier: %c] Delivered %c component!%s", INFO_CLR_SET, component, component, CLR_RST);

                sigprocmask(SIG_UNBLOCK, &block_mask, NULL);

                cap_reached = false;
            }
        } else if (component == 'D') {
            int s_i = find_D();
            if (s_i < 0 && !cap_reached) {
                send_log(msg_id, "%s[Supplier: %c] Not enough space for component %c!%s", WARNING_CLR_SET, component, component, CLR_RST);
                cap_reached = true;
            } else {
                sigset_t block_mask;
                sigemptyset(&block_mask);
                sigaddset(&block_mask, SIGINT);
                sigprocmask(SIG_BLOCK, &block_mask, NULL);

                magazine->buffer[s_i] = component;
                magazine->buffer[s_i + 1] = component;
                magazine->buffer[s_i + 2] = component;
                send_log(msg_id, "%s[Supplier: %c] Delivered %c component!%s", INFO_CLR_SET, component, component, CLR_RST);

                sigprocmask(SIG_UNBLOCK, &block_mask, NULL);

                cap_reached = false;
            }
        }

        while (semop(sem_id, sem_op_out, 2) == -1) {
            if (errno != EINTR) {
                send_log(msg_id, "%s[Supplier: %c] Unable to perform semaphore RELEASE operation! (%s)%s", ERROR_CLR_SET, component, strerror(errno), CLR_RST);
                exit(-1);
            }
        }
    }

    send_log(msg_id, "%s[Supplier: %c] Terminating%s", INFO_CLR_SET, component, CLR_RST);

    shmdt(magazine);

    return 0;
}

void get_component_indexes(char component_type, int *start_out, int *end_out) {
    if (component_type == 'A') {
        *start_out = IsA;
        *end_out = IkA;
        return;
    }
    if (component_type == 'B') {
        *start_out = IsB;
        *end_out = IkB;
        return;
    }
    if (component_type == 'C') {
        *start_out = IsC;
        *end_out = IkC;
        return;
    }
    if (component_type == 'D') {
        *start_out = IsD;
        *end_out = IkD;
        return;
    }

    *start_out = -1;
    *end_out = -1;
    return;
}

void signal_handler(int sig_num, siginfo_t *sig_info, void *data) {
    if (sig_num == SIGINT) {
        send_log(msg_id, "%s[Supplier] Received SIGINT%s", INFO_CLR_SET, CLR_RST);
        is_active = false;
    } else if (sig_num == SIGTERM) {
        if (getppid() != sig_info->si_pid) {
            send_log(msg_id, "%s[Supplier] Something weird is happening... Notifying Director%s", ERROR_CLR_SET, CLR_RST);
            kill(getppid(), SIGUSR2);
        }
    }
}

int find_A() {
    return find_consecutive(magazine->buffer, IsA, IkA, 0, 1);
}

int find_B() {
    return find_consecutive(magazine->buffer, IsB, IkB, 0, 1);
}

int find_C() {
    return find_consecutive(magazine->buffer, IsC, IkC, 0, 2);
}

int find_D() {
    return find_consecutive(magazine->buffer, IsD, IkD, 0, 3);
}


int find_consecutive(char *array, int start_i, int end_i, char search_term, int count) {
    if (count <= 0)
        return -1;

    int consecutive = 0;

    for (int i = start_i; i <= end_i; i++) {
        if (array[i] == search_term)
            consecutive++;

        if (consecutive == count)
            return i - count + 1;
    }

    return -1;
}
