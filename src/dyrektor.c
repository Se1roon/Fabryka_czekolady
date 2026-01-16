#include <pthread.h>
#include <stdbool.h>

#include "../include/common.h"

static int shm_id = -1;
static int sem_id = -1;
static int msg_id = -1;

static Magazine *magazine;

static void signal_handler(int sig_num);

void *handle_user_interface(void *child_processes);

void print_status();
void handle_help_command();

void save_state();
void clean_up();


int main() {
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        fprintf(stderr, "[Director] Failed to assign signal handler to SIGINT! (%s)\n", strerror(errno));
        return -1;
    }

    key_t ipc_key = ftok(".", IPC_KEY_ID);
    if (ipc_key == -1) {
        fprintf(stderr, "[Director] Failed to create key for IPC communication! (%s)\n", strerror(errno));
        return -1;
    }

    shm_id = shmget(ipc_key, sizeof(Magazine), IPC_CREAT | IPC_EXCL | 0600);
    if (shm_id == -1) {
        fprintf(stderr, "[Director] Failed to create Shared Memory segment! (%s)\n", strerror(errno));
        return -1;
    }

    magazine = (Magazine *)shmat(shm_id, NULL, 0);
    if (magazine == (void *)-1) {
        fprintf(stderr, "[Director] Failed to attach to Shared Memory segment! (%s)\n", strerror(errno));
        clean_up();
        return -1;
    }

    sem_id = semget(ipc_key, 1, IPC_CREAT | IPC_EXCL | 0600);
    if (sem_id == -1) {
        fprintf(stderr, "[Director] Failed to create Semaphore Set! (%s)\n", strerror(errno));
        clean_up();
        return -1;
    }

    union semun arg;
    arg.val = 1;
    if (semctl(sem_id, 0, SETVAL, arg) == -1) {
        fprintf(stderr, "[Director] Failed to initialize Semaphore! (%s)\n", strerror(errno));
        clean_up();
        return -1;
    }

    msg_id = msgget(ipc_key, IPC_CREAT | IPC_EXCL | 0600);
    if (msg_id == -1) {
        fprintf(stderr, "[Director] Failed to create Message Queue! (%s)\n", strerror(errno));
        clean_up();
        return -1;
    }

    send_log(msg_id, "[Director] IPC Resources created!");
    send_log(msg_id, "[Director] Spawning child processes...");

    pid_t logger_pid = fork();
    if (logger_pid == -1) {
        fprintf(stderr, "[Director] Failed to create child process! (%s)\n", strerror(errno));
        send_log(msg_id, "[Director] Failed to create child process! (%s)\n", strerror(errno));
        clean_up();
        return -1;
    }
    if (logger_pid == 0) {
        execl("./bin/logger", "./bin/logger", NULL);
        fprintf(stderr, "[Director] Failed to start Logger process! (%s)\n", strerror(errno));
        send_log(msg_id, "[Director] Failed to start Logger process! (%s)\n", strerror(errno));
        clean_up();
        exit(1); // error if execl returned
    }

    pid_t suppliers_pid = fork();
    if (suppliers_pid == -1) {
        fprintf(stderr, "[Director] Failed to create child process! (%s)\n", strerror(errno));
        send_log(msg_id, "[Director] Failed to create child process! (%s)\n", strerror(errno));
        clean_up();
        return -1;
    }
    if (suppliers_pid == 0) {
        execl("./bin/dostawcy", "./bin/dostawcy", NULL);
        fprintf(stderr, "[Director] Failed to start Dostawcy process! (%s)\n", strerror(errno));
        send_log(msg_id, "[Director] Failed to start Dostawcy process! (%s)\n", strerror(errno));
        clean_up();
        exit(1); // error if execl returned
    }

    pid_t factory_pid = fork();
    if (factory_pid == -1) {
        fprintf(stderr, "[Director] Failed to create child process! (%s)\n", strerror(errno));
        send_log(msg_id, "[Director] Failed to create child process! (%s)\n", strerror(errno));
        clean_up();
        return -1;
    }
    if (factory_pid == 0) {
        execl("./bin/fabryka", "./bin/fabryka", NULL);
        fprintf(stderr, "[Director] Failed to start Fabryka process! (%s)\n", strerror(errno));
        send_log(msg_id, "[Director] Failed to start Fabryka process! (%s)\n", strerror(errno));
        clean_up();
        exit(1);
    }

    pid_t child_processes[3] = {suppliers_pid, factory_pid, logger_pid};

    send_log(msg_id, "[Director] Spawned child processes!\n");

    pthread_t interface;
    if ((errno = pthread_create(&interface, NULL, handle_user_interface, (void *)child_processes)) != 0) {
        fprintf(stderr, "[Director] Failed to start user interface! (%s)\n", strerror(errno));
        send_log(msg_id, "[Director] Failed to start user interface! (%s)\n", strerror(errno));
        clean_up();
        return -1;
    }

    // Wait for child processes to finish
    for (int i = 0; i < 3; i++) {
        pid_t child_pid = child_processes[i];

        int status = -1;
        if (waitpid(child_pid, &status, 0) < 0) {
            fprintf(stderr, "[Director] Failed to wait for child termination! (%s)\n", strerror(errno));
            send_log(msg_id, "[Director] Failed to wait for child termination! (%s)\n", strerror(errno));
        }

        if (WIFEXITED(status))
            send_log(msg_id, "[Director] Child process %d has terminated (code %d)\n", child_pid, WEXITSTATUS(status));
    }

    if ((errno = pthread_join(interface, NULL)) != 0) {
        fprintf(stderr, "[Director] Failed to wait for User Interface Thread to finish! (%s)\n", strerror(errno));
        send_log(msg_id, "[Director] Failed to wait for User Interface Thread to finish! (%s)\n", strerror(errno));
    }

    save_state();

    send_log(msg_id, "TERMINATE");
    clean_up();
    return 0;
}

void *handle_user_interface(void *child_processes) {
    pid_t *children = (pid_t *)child_processes;

    char *command = NULL;
    size_t command_len = -1;

    bool factory_work = true;
    bool suppliers_work = true;

    struct sembuf lock = {0, -1, 0};
    struct sembuf unlock = {0, 1, 0};

    printf("\nType help for a list of available commands!\n");
    while (true) {
        printf("\n>");
        fflush(stdout);

        if (getline(&command, &command_len, stdin) != -1) {
            command[command_len-- - 1] = 0; // Remove the trailing newline

            if (strncmp(command, "help", 4) == 0) {
                handle_help_command();
            } else if (strncmp(command, "quit", 4) == 0) {
                kill(children[0], SIGINT);
                kill(children[1], SIGINT);
                kill(children[2], SIGINT);
                break;
            } else if (strncmp(command, "stats", 5) == 0) {
                semop(sem_id, &lock, 1);

                printf("Factory is %s\n", factory_work ? "ON" : "OFF");
                printf("Suppliers is %s\n\n", suppliers_work ? "ON" : "OFF");

                printf("Type1 chocolate produced: %d\n", magazine->type1_produced);
                printf("Type2 chocolate produced: %d\n\n", magazine->type2_produced);

                print_status();

                semop(sem_id, &unlock, 1);
            } else if (strncmp(command, "ts", 2) == 0) {
                kill(children[0], SIGUSR1);
                suppliers_work = !suppliers_work;
                send_log(msg_id, "[Director] Sent %s signal to Suppliers!", suppliers_work ? "ON" : "OFF");
            } else if (strncmp(command, "tf", 2) == 0) {
                kill(children[1], SIGUSR1);
                factory_work = !factory_work;
                send_log(msg_id, "[Director] Sent %s signal to Factory!", factory_work ? "ON" : "OFF");
            } else
                handle_help_command();
        }
    }

    free(command);
    return NULL;
}

void handle_help_command() {
    printf("\nstats - Shows the state of factory and suppliers, magazine state and chocolate produced\n");
    printf("ts	  - Toggles the Suppliers (ON/OFF)\n");
    printf("tf	  - Toggles the Factory (ON/OFF)\n");
    printf("quit  - Quits\n\n");
}

void print_status() {
    printf("\n--- MAGAZINE (Usage: %d/%d) ---\n[ ", magazine->current_usage, MAGAZINE_CAPACITY);
    // Visualize ring buffer linearly
    for (int i = 0; i < MAGAZINE_CAPACITY; i++) {
        char c = magazine->buffer[i];
        printf("%c", c ? c : '.');
    }
    printf(" ]\nHead: %d | Tail: %d\n", magazine->head, magazine->tail);
}

void signal_handler(int sig_num) {
    if (sig_num == SIGINT) {
        save_state();
        send_log(msg_id, "TERMINATE");
        clean_up();
    }

    exit(0);
}

void clean_up() {
    if (shm_id != -1)
        shmctl(shm_id, IPC_RMID, NULL);
    if (sem_id != -1)
        semctl(sem_id, 0, IPC_RMID);
    if (msg_id != -1)
        msgctl(msg_id, IPC_RMID, NULL);

    printf("\n[Director] IPC cleaned up.\n");
}

void save_state() {
    // Save state
    FILE *f = fopen("magazine.bin", "wb");
    if (f) {
        fwrite(magazine, sizeof(Magazine), 1, f);
        fclose(f);
    }

    send_log(msg_id, "[Director] Saved magazine state!");
}
