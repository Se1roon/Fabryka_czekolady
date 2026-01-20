#include <pthread.h>
#include <stdbool.h>

#include "../include/common.h"

static int shm_id = -1;
static int sem_id = -1;
static int msg_id = -1;

static Magazine *magazine;

static void signal_handler(int sig_num);

void *handle_user_interface(void *child_processes);

void handle_help_command();

void restore_state();
void save_state();
void clean_up();

// TODO: Make sure the permissions are minimal
// TODO: Fix the problem when occasionally the magazine will fill with only three components
//		 Maybe create a detecting logic that will temporarily stop deliveries when tail > head to create space
// TODO: Handle child termination on error (modify clean_up)

// TODO: Każdy dostawca jako osobny proces
// TODO: Magazyn podzilony sztywno na regiony A, B, C, D
//		 Założyć ilość czekolady 1 i 2 do wyprodukowania
//		 czekolada 1: A B C = 4 bajty
//		 czekolada 2: A B D = 5 bajtow
//		 wielkosc magazynu = ilosc cz1 * 4 + ilosc cz2 * 5
//		 Ustalic trzeba indeksu od i do ktorych dostawcy daja swoje dane
//		 I skad biora je fabryki
// TODO: Test jak jednego dostawcy zabraknie
// TODO: Przepelnienie kolejki z logami

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

    send_log(msg_id, "[Director] Restoring magazine state...");
    restore_state();

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

    pid_t suppliers[4];
    int i = 0;
    for (char component = 'A'; component < 'A' + 4; component++) {
        pid_t supplier_pid = fork();
        if (supplier_pid == -1) {
            fprintf(stderr, "[Director] Failed to create child process! (%s)\n", strerror(errno));
            send_log(msg_id, "[Director] Failed to create child process! (%s)\n", strerror(errno));
            clean_up();
            return -1;
        }
        if (supplier_pid == 0) {
            char arg[2];
            arg[0] = component;
            arg[1] = 0;
            execl("./bin/dostawcy", "./bin/dostawcy", arg, NULL);
            fprintf(stderr, "[Director] Failed to start Supplier %c process! (%s)\n", component, strerror(errno));
            send_log(msg_id, "[Director] Failed to start Supplier %c process! (%s)\n", component, strerror(errno));
            clean_up();
            exit(1); // error if execl returned
        }
        suppliers[i++] = supplier_pid;
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

    pid_t child_processes[6];
    for (int i = 0; i < 4; i++)
        child_processes[i] = suppliers[i];
    child_processes[4] = factory_pid;
    child_processes[5] = logger_pid;

    send_log(msg_id, "[Director] Spawned child processes!\n");

    pthread_t interface;
    if ((errno = pthread_create(&interface, NULL, handle_user_interface, (void *)child_processes)) != 0) {
        fprintf(stderr, "[Director] Failed to start user interface! (%s)\n", strerror(errno));
        send_log(msg_id, "[Director] Failed to start user interface! (%s)\n", strerror(errno));
        clean_up();
        return -1;
    }

    // Wait for child processes to finish (except Logger - it needs to be still on)
    for (int i = 0; i < 5; i++) {
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
                send_log(msg_id, "[Director] Killing children :O");
                for (int i = 0; i < 5; i++)
                    kill(children[i], SIGINT);
                break;
            } else if (strncmp(command, "stats", 5) == 0) {
                semop(sem_id, &lock, 1);

                printf("Factory is %s\n", factory_work ? "ON" : "OFF");
                printf("Suppliers is %s\n\n", suppliers_work ? "ON" : "OFF");

                printf("Type1 chocolate produced: %d\n", magazine->type1_produced);
                printf("Type2 chocolate produced: %d\n\n", magazine->type2_produced);

                printf("MAGAZINE:\n");
                for (int i = 0; i < MAGAZINE_CAPACITY; i++) {
                    if (i % 100 == 0)
                        printf("\n");
                    if (magazine->buffer[i] == 0)
                        printf(".");
                    else
                        printf("%c", magazine->buffer[i]);
                }

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
        } else {
            send_log(msg_id, "[Director] Killing children :O");
            for (int i = 0; i < 5; i++)
                kill(children[i], SIGINT);
            break; // Exit on CTRL+D or error
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

void signal_handler(int sig_num) {
    if (sig_num == SIGINT) {
        save_state();
        send_log(msg_id, "[Director] Received SIGINT");
        send_log(msg_id, "TERMINATE"); // Terminate logger process
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

void restore_state() {
    FILE *f = fopen("magazine.bin", "r");
    if (f) {
        fwrite(magazine, sizeof(Magazine), 1, f);
        fclose(f);
        send_log(msg_id, "[Director] Restored magazine state!");
    } else
        send_log(msg_id, "[Director] magazine.bin file not found. Nothing to restore from.");
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
