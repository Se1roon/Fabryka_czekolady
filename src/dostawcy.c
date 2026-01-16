#include <sched.h>
#include <stdbool.h>

#include "../include/common.h"

static int msg_id = -1;
static bool is_active = true;

static void signal_handler(int sig_num);

int main() {
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        fprintf(stderr, "[Suppliers] Failed to assign signal handler to SIGINT! (%s)\n", strerror(errno));
        return -1;
    }
    if (signal(SIGUSR1, signal_handler) == SIG_ERR) {
        fprintf(stderr, "[Suppliers] Failed to assign signal handler to SIGINT! (%s)\n", strerror(errno));
        return -1;
    }


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

    srand(time(NULL));

    send_log(msg_id, "[Suppliers] Starting deliveries!");

    while (1) {
        if (is_active) {
            // Randomly pick a product
            int r = rand() % 4;
            char item = 'A' + r;
            int size = GET_SIZE(item);
            int idx = GET_TYPE_IDX(item);

            // LOCK
            if (semop(sem_id, &lock, 1) == -1)
                break;

            int free_space = MAGAZINE_CAPACITY - magazine->current_usage;

            bool keep_small_components = true;
            if (size == 1 && free_space < SMALL_COMPONENTS_TRESHOLD)
                keep_small_components = false;


            bool slots_check = magazine->component_counts[idx] * size + size <= SLOTS_LIMIT;
            bool space_check = free_space >= size;

            // Check if there is space in the Ring
            if (keep_small_components && slots_check && space_check) {
                // Push to Ring (wrapping around)
                for (int i = 0; i < size; i++) {
                    magazine->buffer[magazine->tail] = item;
                    magazine->tail = (magazine->tail + 1) % MAGAZINE_CAPACITY;
                }
                magazine->current_usage += size;
                magazine->component_counts[idx]++;

                // Log only occasionally to avoid spamming the file
                if (rand() % 10 == 0)
                    send_log(msg_id, "Supplier delivered %c (Size %d)", item, size);
            }

            // UNLOCK
            semop(sem_id, &unlock, 1);

            sched_yield();
        }
    }

    send_log(msg_id, "[Suppliers] Stopping.");
    shmdt(magazine);

    return 0;
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
