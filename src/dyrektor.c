#include <pthread.h>
#include <stdbool.h>

#include "../include/common.h"


static int shm_id = -1;
static int sem_id = -1;
static int msg_id = -1;
static Magazine *magazine;

static pthread_t process_manager;
static pid_t child_processes[7];

static bool workerX_active = true;
static bool workerY_active = true;

static void signal_handler(int sig_num, siginfo_t *sig_info, void *data);
void *manage_processes(void *arg);

void restore_state();
void save_state();
void clean_up_IPC();
void clean_up_CHILDREN(pid_t *child_processes, int count, bool force);


int main() {
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        fprintf(stderr, "%s[Director] Failed to assign signal handler to SIGINT! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        fprintf(stderr, "%s[Director] Failed to assign signal handler to SIGUSR1! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        fprintf(stderr, "%s[Director] Failed to assign signal handler to SIGUSR2! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }

    key_t ipc_key = ftok(".", IPC_KEY_ID);
    if (ipc_key == -1) {
        fprintf(stderr, "%s[Director] Failed to create key for IPC communication! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }

    shm_id = shmget(ipc_key, sizeof(Magazine), IPC_CREAT | IPC_EXCL | 0600);
    if (shm_id == -1) {
        fprintf(stderr, "%s[Director] Failed to create Shared Memory segment! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        return -1;
    }

    magazine = (Magazine *)shmat(shm_id, NULL, 0);
    if (magazine == (void *)-1) {
        fprintf(stderr, "%s[Director] Failed to attach to Shared Memory segment! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        clean_up_IPC();
        return -1;
    }

    sem_id = semget(ipc_key, 1, IPC_CREAT | IPC_EXCL | 0600);
    if (sem_id == -1) {
        fprintf(stderr, "%s[Director] Failed to create Semaphore Set! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        clean_up_IPC();
        return -1;
    }

    // Initialize Semaphore to 1
    union semun arg;
    arg.val = 1;
    if (semctl(sem_id, 0, SETVAL, arg) == -1) {
        fprintf(stderr, "%s[Director] Failed to initialize Semaphore! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        clean_up_IPC();
        return -1;
    }

    msg_id = msgget(ipc_key, IPC_CREAT | IPC_EXCL | 0600);
    if (msg_id == -1) {
        fprintf(stderr, "%s[Director] Failed to create Message Queue! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        clean_up_IPC();
        return -1;
    }

    send_log(msg_id, "%s[Director] IPC Resources created!%s", INFO_CLR_SET, CLR_RST);

    send_log(msg_id, "%s[Director] Restoring magazine state...%s", INFO_CLR_SET, CLR_RST);
    restore_state();
    magazine->typeX_produced = 0;
    magazine->typeY_produced = 0;

    send_log(msg_id, "%s[Director] Creating thread for managing processes...%s", INFO_CLR_SET, CLR_RST);

    if ((errno = pthread_create(&process_manager, NULL, manage_processes, NULL)) != 0) {
        send_log(msg_id, "%s[Director] Failed to create thread for process management! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);
        clean_up_IPC();
        return -1;
    }

    send_log(msg_id, "%s[Director] Process Management thread launched successfully!%s", INFO_CLR_SET, CLR_RST);

    send_log(msg_id, "%s[Director] Spawning child processes...%s", INFO_CLR_SET, CLR_RST);

    // Creating Logger process
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

    int children_count = 0;
    child_processes[children_count++] = logger_pid;

    // Creating Supplier processes
    for (char component = 'A'; component < 'A' + 4; component++) {
        pid_t supplier_pid = fork();
        if (supplier_pid == -1) {
            send_log(msg_id, "%s[Director] Failed to create child process! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);

            clean_up_CHILDREN(child_processes, children_count, true);
            clean_up_IPC();
            return -1;
        }
        if (supplier_pid == 0) {
            char arg[2];
            arg[0] = component;
            arg[1] = 0;
            execl("./bin/dostawca", "./bin/dostawca", arg, NULL);
            send_log(msg_id, "%s[Director] Failed to start Supplier %c process! (%s)%s", ERROR_CLR_SET, component, strerror(errno), CLR_RST);

            clean_up_CHILDREN(child_processes, children_count, true);
            clean_up_IPC();
            exit(1);
        }
        child_processes[children_count++] = supplier_pid;
    }

    // Creating Worker processes
    for (char worker_type = 'X'; worker_type <= 'Y'; worker_type++) {
        pid_t worker_pid = fork();
        if (worker_pid == -1) {
            send_log(msg_id, "%s[Director] Failed to create child process! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);

            clean_up_CHILDREN(child_processes, children_count, true);
            clean_up_IPC();
            return -1;
        }
        if (worker_pid == 0) {
            char arg[2];
            arg[0] = worker_type;
            arg[1] = 0;
            execl("./bin/pracownik", "./bin/pracownik", arg, NULL);
            send_log(msg_id, "%s[Director] Failed to start Worker %c process! (%s)%s", ERROR_CLR_SET, worker_type, strerror(errno), CLR_RST);

            clean_up_CHILDREN(child_processes, children_count, true);
            clean_up_IPC();
            exit(1);
        }
        child_processes[children_count++] = worker_pid;
    }

    send_log(msg_id, "%s[Director] Spawned child processes!%s", INFO_CLR_SET, CLR_RST);

    send_log(msg_id, "%s[Director] Waiting for Factory to finish work!%s", INFO_CLR_SET, CLR_RST);

    if ((errno = pthread_join(process_manager, NULL)) != 0) {
        send_log(msg_id, "%s[Director] Failed to wait for child processes! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);
        clean_up_CHILDREN(child_processes, children_count, true);
        clean_up_IPC();
        return -1;
    }

    send_log(msg_id, "%s[Director] FACTORY FINISHED WORK!%s", INFO_CLR_SET, CLR_RST);

    save_state();

    kill(logger_pid, SIGINT);

    int status = -1;
    if (waitpid(logger_pid, &status, 0) < 0)
        fprintf(stderr, "%s[Director] Failed to wait for Logger termination! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);

    if (WIFEXITED(status))
        fprintf(stderr, "%s[Director] Child process %d has terminated (code %d)%s\n", INFO_CLR_SET, logger_pid, WEXITSTATUS(status), CLR_RST);

    clean_up_IPC();
    return 0;
}

void signal_handler(int sig_num, siginfo_t *sig_info, void *data) {
    if (sig_num == SIGINT) {
        save_state();
        send_log(msg_id, "%s[Director] Received SIGINT%s", INFO_CLR_SET, CLR_RST);
        clean_up_CHILDREN(child_processes, 7, true);
        clean_up_IPC();
        exit(0);
    } else if (sig_num == SIGUSR1) {
        if (sig_info->si_pid == child_processes[5]) {
            // source is Worker X -> X Production is done
            if (!workerY_active) {
                send_log(msg_id, "%s[Director] Stopping A deliveries!%s", WARNING_CLR_SET, CLR_RST);
                kill(child_processes[1], SIGINT);
                send_log(msg_id, "%s[Director] Stopping B deliveries!%s", WARNING_CLR_SET, CLR_RST);
                kill(child_processes[2], SIGINT);
            }
            send_log(msg_id, "%s[Director] Stopping C deliveries!%s", WARNING_CLR_SET, CLR_RST);
            kill(child_processes[3], SIGINT);
            workerX_active = false;
        } else if (sig_info->si_pid == child_processes[6]) {
            // source is Worker Y -> Y Production is done
            if (!workerX_active) {
                send_log(msg_id, "%s[Director] Stopping A deliveries!%s", WARNING_CLR_SET, CLR_RST);
                kill(child_processes[1], SIGINT);
                send_log(msg_id, "%s[Director] Stopping B deliveries!%s", WARNING_CLR_SET, CLR_RST);
                kill(child_processes[2], SIGINT);
            }
            send_log(msg_id, "%s[Director] Stopping D deliveries!%s", WARNING_CLR_SET, CLR_RST);
            kill(child_processes[4], SIGINT);
            workerY_active = false;
        }
    } else if (sig_num == SIGUSR2) {
        send_log(msg_id, "%s[Director] Received SIGUSR2... A terrorist attack detected!%s", ERROR_CLR_SET, CLR_RST);
        clean_up_CHILDREN(child_processes, 7, true);
        clean_up_IPC();
        exit(0);
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

void clean_up_CHILDREN(pid_t *child_processes, int count, bool force) {
    if (force) {
        pthread_cancel(process_manager);
        pthread_join(process_manager, NULL);

        for (int i = count - 1; i >= 1; i--) {
            if (child_processes[i] == 0)
                continue;
            kill(child_processes[i], SIGINT);
        }
    }

    for (int i = count - 1; i >= 1; i--) {
        pid_t child_pid = child_processes[i];
        if (child_pid == 0)
            continue;

        int status = -1;
        if (waitpid(child_pid, &status, 0) < 0) {
            if (errno != ECHILD)
                send_log(msg_id, "%s[Director] Failed to wait for child termination! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);
        } else if (WIFEXITED(status))
            send_log(msg_id, "%s[Director] Child process %d has terminated (code %d)%s", ERROR_CLR_SET, child_pid, WEXITSTATUS(status), CLR_RST);

        child_processes[i] = 0;
    }

    if (force)
        if (child_processes[0] != 0)
            kill(child_processes[0], SIGINT);

    if (child_processes[0] != 0) {
        int status = -1;
        if (waitpid(child_processes[0], &status, 0) < 0) {
            if (errno != ECHILD)
                send_log(msg_id, "%s[Director] Failed to wait for child termination! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);
        } else if (WIFEXITED(status))
            send_log(msg_id, "%s[Director] Child process %d has terminated (code %d)%s", ERROR_CLR_SET, child_processes[0], WEXITSTATUS(status), CLR_RST);
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

void *manage_processes(void *arg) {
    int p_status;
    pid_t p_pid;
    int collected = 0;
    while (collected < 6) { // Doesn't wait for logger
        if ((p_pid = waitpid(-1, &p_status, WNOHANG)) > 0) {
            send_log(msg_id, "%s[Process Manager] Successfully collected process %d (exit code %d)!%s", INFO_CLR_SET, p_pid, p_status, CLR_RST);
            collected++;

            for (int i = 0; i < 7; i++) {
                if (child_processes[i] == p_pid) {
                    child_processes[i] = 0;
                    break;
                }
            }
        }
    }

    return NULL;
}
