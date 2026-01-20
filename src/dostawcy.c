#include <sched.h>
#include <stdbool.h>

#include "../include/common.h"

static int msg_id = -1;
static bool is_active = true;

void get_component_indexes(char component_type, int *start_out, int *end_out);

static void signal_handler(int sig_num);

int main(int argc, char *argv[]) {
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        fprintf(stderr, "[Suppliers] Failed to assign signal handler to SIGINT! (%s)\n", strerror(errno));
        return -1;
    }
    if (signal(SIGUSR1, signal_handler) == SIG_ERR) {
        fprintf(stderr, "[Suppliers] Failed to assign signal handler to SIGINT! (%s)\n", strerror(errno));
        return -1;
    }

    if (argc != 2) {
        fprintf(stderr, "[Supplier] Invalid arguments count!\n");
        return -1;
    }
    // Handle incorrect number of arguments itp
    char component = argv[1][0];

    key_t ipc_key = ftok(".", IPC_KEY_ID);
    if (ipc_key == -1) {
        fprintf(stderr, "[Suppliers] Failed to create key for IPC communication! (%s)\n", strerror(errno));
        return -1;
    }

    int shm_id = shmget(ipc_key, sizeof(Magazine), IPC_CREAT | 0600);
    if (shm_id == -1) {
        fprintf(stderr, "[Suppliers] Failed to join the Shared Memory segment! (%s)\n", strerror(errno));
        return -1;
    }

    int sem_id = semget(ipc_key, 1, IPC_CREAT | 0600);
    if (sem_id == -1) {
        fprintf(stderr, "[Suppliers] Failed to join the Semaphore Set! (%s)\n", strerror(errno));
        return -1;
    }

    msg_id = msgget(ipc_key, IPC_CREAT | 0600);
    if (msg_id == -1) {
        fprintf(stderr, "[Suppliers] Failed to join the Message Queue! (%s)\n", strerror(errno));
        return -1;
    }

    Magazine *magazine = (Magazine *)shmat(shm_id, NULL, 0);
    if (magazine == (void *)-1) {
        fprintf(stderr, "[Suppliers] Failed to attach to Shared Memory segment! (%s)\n", strerror(errno));
        return -1;
    }

    struct sembuf lock = {0, -1, 0};
    struct sembuf unlock = {0, 1, 0};

    int start_index;
    int end_index;
    get_component_indexes(component, &start_index, &end_index);

    send_log(msg_id, "[Supplier: %c] Starting deliveries of %c!", component, component);
    while (1) {
        semop(sem_id, &lock, 1);

        for (int i = start_index; i <= end_index; i++) {
            if (magazine->buffer[i] == 0) {
                magazine->buffer[i] = component;
                send_log(msg_id, "[Supplier: %c] Delivered %c component!", component, component);
                break;
            }
        }

        semop(sem_id, &unlock, 1);
    }

    send_log(msg_id, "[Supplier: %c] Stopping.", component);

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
        send_log(msg_id, "[Suppliers] Received SIGINT... Terminating!\n");
        exit(0);
    }
    if (sig_num == SIGUSR1) {
        is_active = !is_active;
        send_log(msg_id, "[Suppliers] Received %s (SIGUSR1)... Switching!\n", is_active ? "ON" : "OFF");
    }
}
