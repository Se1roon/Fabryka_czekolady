#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "fabryka.h"

// TODO: Implement synchronized logging to file
// TODO: Implement saving "magazyn" state to a file on signal


magazyn_t magazyn;

bool magazine_open = true;
bool slaves_work = true;

// #define TEST_RESTORE

int main(void) {
	pthread_t s1_pid;	// TID of stanowisko_1
	pthread_t s2_pid;	// TID of stanowisko_2
	pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	unsigned char terrn = 1;	// Number indicating which thread (or signal) is responsible for the error 
								// 1 - stanowisko_1 (or SIGUSR1)
								// 2 - stanowisko_2 (or SIGUSR2)

	#ifdef TEST_RESTORE
		if (restore_state() != 0) {
			fprintf(stderr, "[Fabryka][main][ERRN] Something went wrong while restoring magazine's state!\n");
			return 5;
		}
	#else
		magazyn.capacity = 210;
		magazyn.a_count = 10000000;
		magazyn.b_count = 10000000;
		magazyn.c_count = 10000000;
		magazyn.d_count = 10000000;
	#endif

	struct sigaction sig_action;
	sig_action.sa_handler = sig_handler;
	sigemptyset(&sig_action.sa_mask);
	sig_action.sa_flags = SA_RESTART;

	if (sigaction(SIGUSR1, &sig_action, NULL) != 0) {
		terrn = 1;
		goto errn_sig_handler;
	}
	if (sigaction(SIGUSR2, &sig_action, NULL) != 0) {
		terrn = 2;
		goto errn_sig_handler;
	}

	if (pthread_create(&s1_pid, NULL, stanowisko_1, (void *)&lock) != 0) {
		terrn = 1;
		goto errn_thread_create;
	}
	if (pthread_create(&s2_pid, NULL, stanowisko_2, (void *)&lock) != 0) {
		terrn = 2;
		goto errn_thread_create;
	}

	void *ret_val = NULL;

	if (pthread_join(s1_pid, &ret_val) != 0) {
		terrn = 1;
		goto errn_thread_join;
	}
	printf("\n[Fabryka][main][INFO] Thread (stanowisko_1) exited with code %ld\n", (long)ret_val);

	if (pthread_join(s2_pid, &ret_val) != 0) {
		terrn = 2;
		goto errn_thread_join;
	}
	printf("[Fabryka][main][INFO] Thread (stanowisko_2) exited with code %ld\n", (long)ret_val);

	printf("\nStan magazynu: \n");
	printf("A: %zu\n", magazyn.a_count);
	printf("B: %zu\n", magazyn.b_count);
	printf("C: %zu\n", magazyn.c_count);
	printf("D: %zu\n", magazyn.d_count);

	if (save_state() != 0) {
		fprintf(stderr, "[Fabryka][main][ERRN] Something went wrong while saving magazine's state!\n");
		return 4;
	}

	return 0;

	// Errors
errn_sig_handler:
	fprintf(stderr, "[Fabryka][SIG][ERRN] Can't catch signal SIGUSR%d\n", terrn);
	return 1;
errn_thread_create:
	fprintf(stderr, "[Fabryka][main][ERRN] Failed to create thread (stanowisko_%u)!\n", terrn);
	return 2;
errn_thread_join:
	fprintf(stderr, "[Fabryka][main][ERRN] Failed to join thread (stanowisko_%u)[TID=%lu]\n", terrn, terrn == 1 ? s1_pid : s2_pid);
	return 3;
}


void *stanowisko_1(void *lock) {
	pthread_mutex_t *mutex_lock = (pthread_mutex_t *)lock;
	unsigned long errn_code = 0;	// 0 - success, no error
									// other - error

	while (slaves_work) {
		if (magazine_open) {
			// Acquire the Lock
			if (pthread_mutex_lock(mutex_lock) != 0) {
				fprintf(stderr, "[Fabryka][stanowisko 1][TID=%lu][ERRN] Failed to acquire lock!\n", pthread_self());
				return (void *)1;
			}

			// Do stuff on Critical Section
			if (magazyn.a_count > 0 && magazyn.b_count > 0 && magazyn.c_count > 0) {
				magazyn.a_count--;
				magazyn.b_count--;
				magazyn.c_count--;
			} else {
				errn_code = 2;
				goto release_lock; // Could omit this because it will flow naturally into it, but
								   // something in the future might go between goto and the label
			}

			// Release the Lock
			release_lock:
				if (pthread_mutex_unlock(mutex_lock) != 0) {
					fprintf(stderr, "[Fabryka][stanowisko 1][TID=%lu][ERRN] Failed to release the lock!\n", pthread_self());
					return (void *)1;
				}

				if (errn_code != 0) {
					printf("[Fabryka][stanowisko 1][WARN] Brak składników w magazynie!\n");
					return (void *)errn_code;
				}

				printf("[Fabryka][stanowisko 1][INFO] Wytworzono czekoladę typu 1!\n");
		} else
			printf("[Fabryka][stanowisko 1][INFO] Magazine closed, can't produce!\n");
	}

	return (void *)0;
}

void *stanowisko_2(void *lock) {
	pthread_mutex_t *mutex_lock = (pthread_mutex_t *)lock;
	unsigned long errn_code = 0;	// 0 - success, no error
									// other - error

	while (slaves_work) {
		if (magazine_open) {
			// Acquire the Lock
			if (pthread_mutex_lock(mutex_lock) != 0) {
				fprintf(stderr, "[Fabryka][stanowisko 2][TID=%lu][ERRN] Failed to acquire lock!\n", pthread_self());
				return (void *)1;
			}

			// Do stuff on Critical Section
			if (magazyn.a_count > 0 && magazyn.b_count > 0 && magazyn.d_count > 0) {
				magazyn.a_count--;
				magazyn.b_count--;
				magazyn.d_count--;
			} else {
				errn_code = 2;
				goto release_lock; // Could omit this because it will flow naturally into it, but
								   // something in the future might go between goto and the label
			}

			release_lock:
				if (pthread_mutex_unlock(mutex_lock) != 0) {
					fprintf(stderr, "[Fabryka][stanowisko 2][TID=%lu][ERRN] Failed to release the lock!\n", pthread_self());
					return (void *)1;
				}

				if (errn_code != 0) {
					printf("[Fabryka][stanowisko 2][WARN] Brak składników w magazynie!\n");
					return (void *)errn_code;
				}

				printf("[Fabryka][stanowisko 2][INFO] Wytworzono czekoladę typu 2!\n");
		} else
			printf("[Fabryka][stanowisko 2][INFO] Magazine closed, can't produce!\n");
	}

	return (void *)0;
}

int restore_state() {
	int fd = open(SAVE_FILE, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "[Fabryka][RESTORE][ERRN] Unable to open file '%s'\n", SAVE_FILE);
		return 1;
	}

	if (read(fd, &magazyn, sizeof(magazyn)) < 0) {
		fprintf(stderr, "[Fabryka][RESTORE][ERRN] Unable to read from file '%s'\n", SAVE_FILE);
		close(fd);
		return 2;
	}

	close(fd);
	return 0;
}

int save_state() {
	int fd = open(SAVE_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		fprintf(stderr, "[Fabryka][SAVE][ERRN] Unable to open file '%s'\n", SAVE_FILE);
		return 1;
	}

	if (write(fd, &magazyn, sizeof(magazyn)) < 0) {
		fprintf(stderr, "[Fabryka][SAVE][ERRN] Unable to write to file '%s'\n", SAVE_FILE);
		close(fd);
		return 2;
	}

	close(fd);
	return 0;
}

static void sig_handler(int signo) {
	// Using 'write' syscall instead of just printf to avoid deadlock
	if (signo == SIGUSR1) {
		write(1, "[Fabryka][SIG][INFO] Received signal: polecenie_1\n", 50);
		slaves_work = false;
	} else if (signo == SIGUSR2) {
		write(1, "[Fabryka][SIG][INFO] Received signal: polecenie_2\n", 50);
		magazine_open = !magazine_open;
	} else {
		write(1, "[Fabryka][SIG][WARN] Received unknown signal\n", 45);
	}

	return;
}

