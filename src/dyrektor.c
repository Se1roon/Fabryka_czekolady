#include <pthread.h>
#include <stdbool.h>

#include "../include/common.h"


static int shm_id = -1;
static int sem_id = -1;
static int msg_id = -1;
static Magazine *magazine;

static pid_t child_processes[6];

static bool workerX_active = true;
static bool workerY_active = true;

static void signal_handler(int sig_num);

void restore_state();
void save_state();
void clean_up_IPC();
void clean_up_CHILD(pid_t *child_processes, int count, bool force);

// TODO: Logowanie na stdout (tee ?)
// TODO: Koloryzowanie logow

// TODO:

int main() {
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        fprintf(stderr, "[Director] Failed to assign signal handler to SIGINT! (%s)\n", strerror(errno));
        return -1;
    }
    if (signal(SIGUSR1, signal_handler) == SIG_ERR) {
        fprintf(stderr, "[Director] Failed to assign signal handler to SIGUSR1! (%s)\n", strerror(errno));
        return -1;
    }
    if (signal(SIGUSR2, signal_handler) == SIG_ERR) {
        fprintf(stderr, "[Director] Failed to assign signal handler to SIGUSR2! (%s)\n", strerror(errno));
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
        clean_up_IPC();
        return -1;
    }

    sem_id = semget(ipc_key, 1, IPC_CREAT | IPC_EXCL | 0600);
    if (sem_id == -1) {
        fprintf(stderr, "[Director] Failed to create Semaphore Set! (%s)\n", strerror(errno));
        clean_up_IPC();
        return -1;
    }

    union semun arg;
    arg.val = 1;
    if (semctl(sem_id, 0, SETVAL, arg) == -1) {
        fprintf(stderr, "[Director] Failed to initialize Semaphore! (%s)\n", strerror(errno));
        clean_up_IPC();
        return -1;
    }

    msg_id = msgget(ipc_key, IPC_CREAT | IPC_EXCL | 0600);
    if (msg_id == -1) {
        fprintf(stderr, "[Director] Failed to create Message Queue! (%s)\n", strerror(errno));
        clean_up_IPC();
        return -1;
    }

    send_log(msg_id, "%s[Director] IPC Resources created!%s", INFO_CLR_SET, CLR_RST);
    send_log(msg_id, "%s[Director] Restoring magazine state...%s", INFO_CLR_SET, CLR_RST);
    restore_state();
    send_log(msg_id, "%s[Director] Spawning child processes...%s", INFO_CLR_SET, CLR_RST);

    pid_t logger_pid = fork();
    if (logger_pid == -1) {
        send_log(msg_id, "%s[Director] Failed to create child process! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);
        clean_up_IPC();
        return -1;
    }
    if (logger_pid == 0) {
        execl("./bin/logger", "./bin/logger", NULL);
        send_log(msg_id, "%s[Director] Failed to start Logger process! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);
        clean_up_IPC();
        return -1;
    }

    int i = 0;
    child_processes[i++] = logger_pid;

    for (char component = 'A'; component < 'A' + 4; component++) {
        pid_t supplier_pid = fork();
        if (supplier_pid == -1) {
            send_log(msg_id, "%s[Director] Failed to create child process! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);

            clean_up_CHILD(child_processes, i, true);
            clean_up_IPC();
            return -1;
        }
        if (supplier_pid == 0) {
            char arg[2];
            arg[0] = component;
            arg[1] = 0;
            execl("./bin/dostawcy", "./bin/dostawcy", arg, NULL);
            send_log(msg_id, "%s[Director] Failed to start Supplier %c process! (%s)%s", ERROR_CLR_SET, component, strerror(errno), CLR_RST);

            clean_up_CHILD(child_processes, i, true);
            clean_up_IPC();
            exit(1);
        }
        child_processes[i++] = supplier_pid;
    }

    pid_t factory_pid = fork();
    if (factory_pid == -1) {
        send_log(msg_id, "%s[Director] Failed to create child process! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);

        clean_up_CHILD(child_processes, i, true);
        clean_up_IPC();
        return -1;
    }
    if (factory_pid == 0) {
        execl("./bin/fabryka", "./bin/fabryka", NULL);
        send_log(msg_id, "%s[Director] Failed to start Factory process! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);

        clean_up_CHILD(child_processes, i, true);
        clean_up_IPC();
        exit(1);
    }
    child_processes[i++] = factory_pid;

    send_log(msg_id, "%s[Director] Spawned child processes!%s\n", INFO_CLR_SET, CLR_RST);

    clean_up_CHILD(child_processes, i, false);

    save_state();

    kill(logger_pid, SIGINT);

    int status = -1;
    if (waitpid(logger_pid, &status, 0) < 0)
        fprintf(stderr, "[Director] Failed to wait for Logger termination! (%s)\n", strerror(errno));

    if (WIFEXITED(status))
        fprintf(stderr, "[Director] Child process %d has terminated (code %d)\n", logger_pid, WEXITSTATUS(status));

    clean_up_IPC();
    return 0;
}

void signal_handler(int sig_num) {
    if (sig_num == SIGINT) {
        save_state();
        send_log(msg_id, "%s[Director] Received SIGINT%s", INFO_CLR_SET, CLR_RST);
        clean_up_CHILD(child_processes, 6, true);
        clean_up_IPC();
        exit(0);
    } else if (sig_num == SIGUSR1) {
        // X Production is done
        if (!workerY_active) {
            send_log(msg_id, "%s[Director] Stopping A deliveries!%s", WARNING_CLR_SET, CLR_RST);
            kill(child_processes[1], SIGINT); // Terminate A supplier
            send_log(msg_id, "%s[Director] Stopping B deliveries!%s", WARNING_CLR_SET, CLR_RST);
            kill(child_processes[2], SIGINT); // B
        }
        send_log(msg_id, "%s[Director] Stopping C deliveries!%s", WARNING_CLR_SET, CLR_RST);
        kill(child_processes[3], SIGINT); // C
        workerX_active = false;
    } else if (sig_num == SIGUSR2) {
        // Y production is done
        if (!workerX_active) {
            send_log(msg_id, "%s[Director] Stopping A deliveries!%s", WARNING_CLR_SET, CLR_RST);
            kill(child_processes[1], SIGINT); // Terminate A supplier
            send_log(msg_id, "%s[Director] Stopping B deliveries!%s", WARNING_CLR_SET, CLR_RST);
            kill(child_processes[2], SIGINT); // B
        }
        send_log(msg_id, "%s[Director] Stopping D deliveries!%s", WARNING_CLR_SET, CLR_RST);
        kill(child_processes[4], SIGINT); // 4
        workerY_active = false;
    }
}

void clean_up_IPC() {
    if (shm_id != -1)
        shmctl(shm_id, IPC_RMID, NULL);
    if (sem_id != -1)
        semctl(sem_id, 0, IPC_RMID);
    if (msg_id != -1)
        msgctl(msg_id, IPC_RMID, NULL);

    printf("\n%s[Director] IPC cleaned up.%s\n", INFO_CLR_SET, CLR_RST);
}

void clean_up_CHILD(pid_t *child_processes, int count, bool force) {
    int end = 1;

    if (force) {
        for (int i = count - 1; i >= 0; i--)
            kill(child_processes[i], SIGINT);
        end = 0; // Don't want to wait for logger termination when not forced.
    }

    for (int i = count - 1; i >= end; i--) {
        pid_t child_pid = child_processes[i];

        int status = -1;
        if (waitpid(child_pid, &status, 0) < 0)
            send_log(msg_id, "%s[Director] Failed to wait for child termination! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);

        if (WIFEXITED(status))
            send_log(msg_id, "%s[Director] Child process %d has terminated (code %d)%s", ERROR_CLR_SET, child_pid, WEXITSTATUS(status), CLR_RST);
    }
}

void restore_state() {
    FILE *f = fopen("magazine.bin", "r");
    if (f) {
        fwrite(magazine, sizeof(Magazine), 1, f);
        fclose(f);
        send_log(msg_id, "%s[Director] Restored magazine state!%s", INFO_CLR_SET, CLR_RST);
    } else
        send_log(msg_id, "%s[Director] magazine.bin file not found. Nothing to restore from.%s", WARNING_CLR_SET, CLR_RST);
}

void save_state() {
    // Save state
    FILE *f = fopen("magazine.bin", "wb");
    if (f) {
        fwrite(magazine, sizeof(Magazine), 1, f);
        send_log(msg_id, "%s[Director] Saved magazine state!%s", INFO_CLR_SET, CLR_RST);
        fclose(f);
    }
}
