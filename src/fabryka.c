#include <sched.h>
#include <stdbool.h>

#include "../include/common.h"

typedef struct {
    int has_A;
    int has_B;
    int has_C;
    int has_D;
} Inventory;

static pid_t workers[2];
static int msg_id = -1;
static bool is_active = true;

static void signal_handler_parent(int sig_num);
static void signal_handler_children(int sig_num);

void worker(int type, int shm_id, int sem_id, int msg_id);


int main() {
    if (signal(SIGINT, signal_handler_parent) == SIG_ERR) {
        fprintf(stderr, "[Factory] Failed to assign signal handler to SIGINT! (%s)\n", strerror(errno));
        return -1;
    }
    if (signal(SIGUSR1, signal_handler_parent) == SIG_ERR) {
        fprintf(stderr, "[Factory] Failed to assign signal handler to SIGUSR1! (%s)\n", strerror(errno));
        return -1;
    }

    key_t ipc_key = ftok(".", IPC_KEY_ID);
    if (ipc_key == -1) {
        fprintf(stderr, "[Factory] Failed to create key for IPC communication! (%s)\n", strerror(errno));
        return -1;
    }

    int shm_id = shmget(ipc_key, sizeof(Magazine), IPC_CREAT | 0600);
    if (shm_id == -1) {
        fprintf(stderr, "[Factory] Failed to join the Shared Memory segment! (%s)\n", strerror(errno));
        return -1;
    }

    int sem_id = semget(ipc_key, 1, IPC_CREAT | 0600);
    if (sem_id == -1) {
        fprintf(stderr, "[Factory] Failed to join the Semaphore Set! (%s)\n", strerror(errno));
        return -1;
    }

    msg_id = msgget(ipc_key, IPC_CREAT | 0600);
    if (msg_id == -1) {
        fprintf(stderr, "[Factory] Failed to join the Message Queue! (%s)\n", strerror(errno));
        return -1;
    }

    pid_t worker1 = fork();
    if (worker1 == -1) {
        fprintf(stderr, "[Factory] Failed to create Worker 1 process! (%s)\n", strerror(errno));
        send_log(msg_id, "[Factory] Failed to create Worker 1 process! (%s)\n", strerror(errno));
        return -1;
    }
    if (worker1 == 0)
        worker(1, shm_id, sem_id, msg_id);

    pid_t worker2 = fork();
    if (worker2 == -1) {
        fprintf(stderr, "[Factory] Failed to create Worker 2 process! (%s)\n", strerror(errno));
        send_log(msg_id, "[Factory] Failed to create Worker 2 process! (%s)\n", strerror(errno));
        return -1;
    }
    if (worker2 == 0)
        worker(2, shm_id, sem_id, msg_id);

    workers[0] = worker1;
    workers[1] = worker2;

    for (int i = 0; i < 2; i++) {
        int status = -1;
        if (waitpid(workers[i], &status, 0) < 0) {
            fprintf(stderr, "[Factory] Failed to wait for worker termination! (%s)\n", strerror(errno));
            send_log(msg_id, "[Factory] Failed to wait for worker termination! (%s)\n", strerror(errno));
        }

        if (WIFEXITED(status))
            send_log(msg_id, "[Factory] Worker %d has terminated (code %d)\n", i, WEXITSTATUS(status));
    }

    return 0;
}


void worker(int type, int shm_id, int sem_id, int msg_id) {
    if (signal(SIGINT, signal_handler_children) == SIG_ERR) {
        fprintf(stderr, "[Factory: Worker %d] Failed to assign signal handler to SIGINT! (%s)\n", type, strerror(errno));
        exit(-1);
    }
    if (signal(SIGUSR1, signal_handler_children) == SIG_ERR) {
        fprintf(stderr, "[Factory: Worker %d] Failed to assign signal handler to SIGUSR1! (%s)\n", type, strerror(errno));
        exit(-1);
    }

    Magazine *magazine = (Magazine *)shmat(shm_id, NULL, 0);
    if (magazine == (void *)-1) {
        fprintf(stderr, "[Factory] Failed to attach to Shared Memory segment! (%s)\n", strerror(errno));
        send_log(msg_id, "[Factory] Failed to attach to Shared Memory segment! (%s)\n", strerror(errno));
        exit(-1);
    }

    struct sembuf lock = {0, -1, 0};
    struct sembuf unlock = {0, 1, 0};

    Inventory inv = {0};
    int produced = 0;

    send_log(msg_id, "Worker %d started.", type);

    while (1) {
        if (is_active) {
            semop(sem_id, &lock, 1);

            if (magazine->current_usage > 0) {
                // Peek at the Head of the Ring
                char item = magazine->buffer[magazine->head];
                int size = GET_SIZE(item);
                int idx = GET_TYPE_IDX(item);
                bool needed = false;

                // Decision: Do I need this item?
                if (item == 'A' && !inv.has_A)
                    needed = true;
                else if (item == 'B' && !inv.has_B)
                    needed = true;
                else if (type == 1 && item == 'C' && !inv.has_C)
                    needed = true;
                else if (type == 2 && item == 'D' && !inv.has_D)
                    needed = true;

                if (needed) {
                    // CONSUME: Take from Head
                    magazine->head = (magazine->head + size) % MAGAZINE_CAPACITY;
                    magazine->current_usage -= size;
                    magazine->component_counts[idx]--;

                    if (item == 'A')
                        inv.has_A = 1;
                    if (item == 'B')
                        inv.has_B = 1;
                    if (item == 'C')
                        inv.has_C = 1;
                    if (item == 'D')
                        inv.has_D = 1;
                } else {
                    int slots_occupied = magazine->component_counts[idx] * size;
                    bool is_clogged = magazine->current_usage >= MAGAZINE_CAPACITY - 10;
                    bool oversupply = slots_occupied > SLOTS_LIMIT;

                    if (is_clogged || oversupply) {
                        // Remove the component
                        magazine->head = (magazine->head + size) % MAGAZINE_CAPACITY;
                        magazine->current_usage -= size;
                        magazine->component_counts[idx]--;
                    } else {
                        // RECYCLE: Move from Head to Tail (Rotate Ring)
                        for (int k = 0; k < size; k++) {
                            char val = magazine->buffer[(magazine->head + k) % MAGAZINE_CAPACITY];
                            magazine->buffer[(magazine->tail + k) % MAGAZINE_CAPACITY] = val;
                        }
                        magazine->head = (magazine->head + size) % MAGAZINE_CAPACITY;
                        magazine->tail = (magazine->tail + size) % MAGAZINE_CAPACITY;
                    }
                }
            }

            // Check Recipe
            if (type == 1 && inv.has_A && inv.has_B && inv.has_C) {
                produced++;
                magazine->type1_produced++;
                inv.has_A = 0;
                inv.has_B = 0;
                inv.has_C = 0;
                send_log(msg_id, "[Factory] Worker 1 PRODUCED Chocolate (Total: %d)", produced);
            }
            if (type == 2 && inv.has_A && inv.has_B && inv.has_D) {
                produced++;
                magazine->type2_produced++;
                inv.has_A = 0;
                inv.has_B = 0;
                inv.has_D = 0;
                send_log(msg_id, "[Factory] Worker 2 PRODUCED Chocolate (Total: %d)", produced);
            }

            semop(sem_id, &unlock, 1);

            sched_yield();
        }
    }

    send_log(msg_id, "[Factory] Worker %d stopped. Total produced: %d", type, produced);
    shmdt(magazine);
    exit(0);
}

void signal_handler_parent(int sig_num) {
    if (sig_num == SIGINT) {
        kill(workers[0], SIGINT);
        kill(workers[1], SIGINT);
    } else if (sig_num == SIGUSR1) {
        kill(workers[0], SIGUSR1);
        kill(workers[1], SIGUSR1);
    }
}

void signal_handler_children(int sig_num) {
    if (sig_num == SIGINT) {
        send_log(msg_id, "[Factory] Received SIGINT... Terminating\n");
        exit(0);
    }
    if (sig_num == SIGUSR1) {
        is_active = !is_active;
        send_log(msg_id, "[Factory] Received %s (SIGUSR1)... Switching!\n", is_active ? "ON" : "OFF");
    }
}
