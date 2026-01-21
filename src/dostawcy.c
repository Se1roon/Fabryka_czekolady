#include <sched.h>
#include <stdbool.h>

#include "../include/common.h"

static int msg_id = -1;
static bool is_active = true;

static Magazine *magazine = NULL;

void get_component_indexes(char component_type, int *start_out, int *end_out);

static void signal_handler(int sig_num);

int main(int argc, char *argv[]) {
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        fprintf(stderr, "%s[Suppliers] Failed to assign signal handler to SIGINT! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }

    if (argc != 2) {
        fprintf(stderr, "%s[Supplier] Invalid arguments count!%s\n", ERROR_CLR_SET, CLR_RST);
        return -1;
    }

    char component = argv[1][0];

    key_t ipc_key = ftok(".", IPC_KEY_ID);
    if (ipc_key == -1) {
        fprintf(stderr, "%s[Suppliers] Failed to create key for IPC communication! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }

    int shm_id = shmget(ipc_key, sizeof(Magazine), IPC_CREAT | 0600);
    if (shm_id == -1) {
        fprintf(stderr, "%s[Suppliers] Failed to join the Shared Memory segment! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }

    int sem_id = semget(ipc_key, 1, IPC_CREAT | 0600);
    if (sem_id == -1) {
        fprintf(stderr, "%s[Suppliers] Failed to join the Semaphore Set! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }

    msg_id = msgget(ipc_key, IPC_CREAT | 0600);
    if (msg_id == -1) {
        fprintf(stderr, "%s[Suppliers] Failed to join the Message Queue! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }

    magazine = (Magazine *)shmat(shm_id, NULL, 0);
    if (magazine == (void *)-1) {
        fprintf(stderr, "%s[Suppliers] Failed to attach to Shared Memory segment! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }

    struct sembuf lock = {0, -1, 0};
    struct sembuf unlock = {0, 1, 0};

    int start_index;
    int end_index;
    get_component_indexes(component, &start_index, &end_index);

    send_log(msg_id, "%s[Supplier: %c] Starting deliveries of %c!%s", INFO_CLR_SET, component, component, CLR_RST);
    while (is_active) {
        semop(sem_id, &lock, 1);

        for (int i = start_index; i <= end_index; i++) {
            if (magazine->buffer[i] == 0) {
                magazine->buffer[i] = component;
                send_log(msg_id, "%s[Supplier: %c] Delivered %c component!%s", INFO_CLR_SET, component, component, CLR_RST);
                break;
            }
        }

        semop(sem_id, &unlock, 1);

        sched_yield();
    }

    send_log(msg_id, "%s[Supplier: %c] Stopping%s", INFO_CLR_SET, component, CLR_RST);

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

void signal_handler(int sig_num) {
    if (sig_num == SIGINT) {
        send_log(msg_id, "%s[Suppliers] Received SIGINT%s", INFO_CLR_SET, CLR_RST);
        is_active = false;
    }
}
