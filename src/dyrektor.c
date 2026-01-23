#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#include "../include/common.h"

// TODO: Move colors to logger and send the color code as a parameter
// TODO: Documentation & Tests

static int shm_id = -1;
static int sem_id = -1;
static int msg_id = -1;
static Magazine *magazine;

static pthread_t process_manager;
static pid_t child_processes[7];
static pid_t processes_waited[7] = {false};

static bool factory_active = true;
static bool magazine_active = true;
static bool workerX_active = true;
static bool workerY_active = true;

void *manage_processes(void *arg);

int restore_state();
void save_state();
void clean_up_IPC();
void clean_up_CHILDREN(pid_t *child_processes, int count, bool force);

void unblock_signals_for_child() {
	sigset_t set;
	sigemptyset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);
}


int main() {
    if (X_TYPE_TO_PRODUCE < 0 || Y_TYPE_TO_PRODUCE < 0) {
        fprintf(stderr, "%sInvalid amount of chocolate to produce (must be >= 0)%s", ERROR_CLR_SET, CLR_RST);
        return 0;
    }
    if (MAGAZINE_CAPACITY > 30000) {
        fprintf(stderr, "%sDangerous total capacity of the magazine%s", ERROR_CLR_SET, CLR_RST);
        return 0;
    }

	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);

	if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) {
		fprintf(stderr, "%s[Director] Cannot block signals%s", ERROR_CLR_SET, CLR_RST);
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

    sem_id = semget(ipc_key, 9, IPC_CREAT | IPC_EXCL | 0600);
    if (sem_id == -1) {
        fprintf(stderr, "%s[Director] Failed to create Semaphore Set! (%s)%s\n", ERROR_CLR_SET, strerror(errno), CLR_RST);
        clean_up_IPC();
        return -1;
    }

    // Initialize Semaphores
    union semun arg;
    unsigned short values[9];
    values[SEM_MAGAZINE] = 1;
    values[SEM_EMPTY_A] = IkA - IsA + 1;
    values[SEM_EMPTY_B] = IkB - IsB + 1;
    values[SEM_EMPTY_C] = IkC - IsC + 1;
    values[SEM_EMPTY_D] = IkD - IsD + 1;
    values[SEM_FULL_A] = 0;
    values[SEM_FULL_B] = 0;
    values[SEM_FULL_C] = 0;
    values[SEM_FULL_D] = 0;

    arg.array = values;
    if (semctl(sem_id, 0, SETALL, arg) == -1) {
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

    if (restore_state() != 0)
        send_log(msg_id, "%s[Director] Failed to restore magazine state! Starting from nothing%s", WARNING_CLR_SET, CLR_RST);

    // If the simulation was stopped (due to a terrorist attack for example)
    // restore the amount of chocolate that has been produced so far.
    // If the number of chocolate produced is less than the desired amount
    // it is probably a different simulation so reset it.
    if (magazine->typeX_produced >= X_TYPE_TO_PRODUCE)
        magazine->typeX_produced = 0;
    if (magazine->typeY_produced >= Y_TYPE_TO_PRODUCE)
        magazine->typeY_produced = 0;

    send_log(msg_id, "%s[Director] Amount of chocolate produced so far:\n\tX = %d\n\tY = %d%s",
             INFO_CLR_SET, magazine->typeX_produced, magazine->typeY_produced, CLR_RST);


    send_log(msg_id, "%s[Director] Spawning child processes...%s", INFO_CLR_SET, CLR_RST);

    // Creating Logger process
    pid_t logger_pid = fork();
    if (logger_pid == -1) {
        send_log(msg_id, "%s[Director] Failed to create child process! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);
        clean_up_IPC();
        return -1;
    }
    if (logger_pid == 0) {
		unblock_signals_for_child();
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
			unblock_signals_for_child();
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
			unblock_signals_for_child();
            execl("./bin/pracownik", "./bin/pracownik", arg, NULL);
            send_log(msg_id, "%s[Director] Failed to start Worker %c process! (%s)%s", ERROR_CLR_SET, worker_type, strerror(errno), CLR_RST);

            clean_up_CHILDREN(child_processes, children_count, true);
            clean_up_IPC();
            exit(1);
        }
        child_processes[children_count++] = worker_pid;
    }

    send_log(msg_id, "%s[Director] Spawned child processes! Starting process manager thread%s", INFO_CLR_SET, CLR_RST);

    if ((errno = pthread_create(&process_manager, NULL, manage_processes, NULL)) != 0) {
        send_log(msg_id, "%s[Director] Failed to create thread for process management! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);
        clean_up_IPC();
        return -1;
    }

    send_log(msg_id, "%s[Director] Process Management thread launched successfully!%s", INFO_CLR_SET, CLR_RST);

    send_log(msg_id, "%s[Director] Waiting for Factory to finish work!%s", INFO_CLR_SET, CLR_RST);

	struct timespec timeout;
	siginfo_t info;

	while (factory_active) {
		timeout.tv_sec = 0;
		timeout.tv_nsec = 100000000;

		int sig = sigtimedwait(&mask, &info, &timeout);

		if (sig > 0) {
			if (sig == SIGINT) {
				send_log(msg_id, "%s[Director] Received SIGINT | Shutting down%s", INFO_CLR_SET, CLR_RST);
				save_state();
				clean_up_CHILDREN(child_processes, 7, true);
				clean_up_IPC();
				exit(0);

			} else if (sig == SIGUSR1) {
				if (info.si_pid == child_processes[5]) {
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

				} else if (info.si_pid == child_processes[6]) {
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
			} else if (sig == SIGUSR2) {
				send_log(msg_id, "%s[Director] Received SIGUSR2... A terrorist attack detected!%s", ERROR_CLR_SET, CLR_RST);
				save_state();
				clean_up_CHILDREN(child_processes, 7, true);
				clean_up_IPC();
				exit(0);

			} else if (sig == SIGTERM) {
				send_log(msg_id, "%s[Director] Received SIGTERM... %s Magazine!%s",
						 INFO_CLR_SET, (magazine_active) ? "Stopping" : "Activating", CLR_RST);

				struct sembuf magazine_lock = {SEM_MAGAZINE, -1, SEM_UNDO};
				struct sembuf magazine_unlock = {SEM_MAGAZINE, 1, SEM_UNDO};
				bool lock_acquired = true;

				if (magazine_active) {
					while (semop(sem_id, &magazine_lock, 1) == -1) {
						if (errno != EINTR) {
							send_log(msg_id, "%s[Director] Failed to lock the magazine! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);
							lock_acquired = false;
							break;
						}
					}
					if (lock_acquired) {
						send_log(msg_id, "%s[Director] Magazine Stopped!%s", WARNING_CLR_SET, CLR_RST);
						magazine_active = false;
					}

				} else {
					while (semop(sem_id, &magazine_unlock, 1) == -1) {
						if (errno != EINTR) {
							send_log(msg_id, "%s[Director] Failed to unlock the magazine! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);
							lock_acquired = false;
							break;
						}
					}
					if (lock_acquired) {
						send_log(msg_id, "%s[Director] Magazine Activated!%s", WARNING_CLR_SET, CLR_RST);
						magazine_active = true;
					}
				}
			}
		}
	}

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


void clean_up_IPC() {
    printf("%s[Director] Amount of chocolate produced:\n\tX = %d\n\tY = %d\n%s", INFO_CLR_SET, magazine->typeX_produced, magazine->typeY_produced, CLR_RST);
    if (shm_id != -1)
        shmctl(shm_id, IPC_RMID, NULL);
    if (sem_id != -1)
        semctl(sem_id, 0, IPC_RMID);
    if (msg_id != -1)
        msgctl(msg_id, IPC_RMID, NULL);

    printf("%s[Director] IPC cleaned up.%s\n", INFO_CLR_SET, CLR_RST);
}

void clean_up_CHILDREN(pid_t *child_processes, int count, bool force) {
    if (force) {
        pthread_cancel(process_manager);
        pthread_join(process_manager, NULL);

        for (int i = count - 1; i >= 1; i--) {
            if (child_processes[i] == 0 || processes_waited[i])
                continue;
            kill(child_processes[i], SIGINT);
        }
    }

    for (int i = count - 1; i >= 1; i--) {
        pid_t child_pid = child_processes[i];
        if (child_pid == 0 || processes_waited[i])
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

int restore_state() {
    FILE *f = fopen("magazine.bin", "rb");
    if (f) {
        if (fread(magazine, sizeof(Magazine), 1, f) != 1) {
            send_log(msg_id, "%s[Director] Failed to read magazine.bin! (%s)%s", ERROR_CLR_SET, strerror(errno), CLR_RST);
            fclose(f);
            return -1;
        }
        fclose(f);
        send_log(msg_id, "%s[Director] Restored magazine state!%s", INFO_CLR_SET, CLR_RST);
    } else
        send_log(msg_id, "%s[Director] magazine.bin file not found. Nothing to restore from.%s", WARNING_CLR_SET, CLR_RST);

    return 0;
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
        if ((p_pid = waitpid(-1, &p_status, 0)) > 0) {
            send_log(msg_id, "%s[Process Manager] Successfully collected process %d (exit code %d)!%s", INFO_CLR_SET, p_pid, p_status, CLR_RST);
            collected++;

            for (int i = 0; i < 7; i++) {
                if (child_processes[i] == p_pid) {
                    processes_waited[i] = true;
                    break;
                }
            }
        } else {
			if (errno == ECHILD)
				break;
		}
    }

	factory_active = false;

    return NULL;
}
