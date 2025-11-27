#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "fabryka.h"


// TODO: Implement signal handling
// TODO: Initialize git repository
// TODO: Implement saving "magazyn" state to a file on signal


magazyn_t magazyn = {
	.capacity = 210,
	.a_count = 50,
	.b_count = 50,
	.c_count = 25,
	.d_count = 20
};


int main(void) {
	pthread_t s1_pid;	// TID of stanowisko_1
	pthread_t s2_pid;	// TID of stanowisko_2
	pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	unsigned char terrn = 1;	// Number indicating which thread is responsible for the error 
								// 1 - stanowisko_1
								// 2 - stanowisko_2

	terrn = 2;
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

	return 0;

	// Errors
errn_thread_create:
	fprintf(stderr, "[Fabryka][main][ERRN] Failed to create thread (stanowisko_%u)!\n", terrn);
	return 1;
errn_thread_join:
	fprintf(stderr, "[Fabryka][main][ERRN] Failed to join thread (stanowisko_%u)[TID=%lu]\n", terrn, terrn == 1 ? s1_pid : s2_pid);
	return 2;
}


void *stanowisko_1(void *lock) {
	pthread_mutex_t *mutex_lock = (pthread_mutex_t *)lock;
	unsigned long errn_code = 0;	// 0 - success, no error
									// other - error

	for (int i = 0; i < 10; i++) {
		// Acquire the Lock
		if (pthread_mutex_lock(mutex_lock) != 0) {
			fprintf(stderr, "[Fabryka][stanowisko_1][TID=%lu][ERRN] Failed to acquire lock!\n", pthread_self());
			return (void *)1;
		}

		// Do stuff on Critical Section
		if (magazyn.a_count > 0 && magazyn.b_count > 0 && magazyn.c_count > 0) {
			magazyn.a_count--;
			magazyn.b_count--;
			magazyn.c_count--;
		} else {
			errn_code = 2;
			goto release_lock;
		}

		// Release the Lock
		release_lock:
			if (pthread_mutex_unlock(mutex_lock) != 0) {
				fprintf(stderr, "[Fabryka][stanowisko_1][TID=%lu][ERRN] Failed to release the lock!\n", pthread_self());
				return (void *)1;
			}

			if (errn_code != 0) {
				printf("[Fabryka][stanowisko 1][WARN] Brak składników w magazynie!\n");
				return (void *)errn_code;
			}

			printf("[Fabryka][stanowisko 1][INFO] Wyworzono czekoladę typu 1!\n");
	}

	return (void *)0;
}

void *stanowisko_2(void *lock) {
	pthread_mutex_t *mutex_lock = (pthread_mutex_t *)lock;
	unsigned long errn_code = 0;	// 0 - success, no error
									// other - error

	for (int i = 0; i < 10; i++) {
		// Acquire the Lock
		if (pthread_mutex_lock(mutex_lock) != 0) {
			fprintf(stderr, "[Fabryka][stanowisko_2][TID=%lu][ERRN] Failed to acquire lock!\n", pthread_self());
			return (void *)1;
		}

		// Do stuff on Critical Section
		if (magazyn.a_count > 0 && magazyn.b_count > 0 && magazyn.d_count > 0) {
			magazyn.a_count--;
			magazyn.b_count--;
			magazyn.d_count--;
		} else {
			errn_code = 2;
			goto release_lock;
		}

		release_lock:
			if (pthread_mutex_unlock(mutex_lock) != 0) {
				fprintf(stderr, "[Fabryka][stanowisko_2][TID=%lu][ERRN] Failed to release the lock!\n", pthread_self());
				return (void *)1;
			}

			if (errn_code != 0) {
				printf("[Fabryka][stanowisko 2][WARN] Brak składników w magazynie!\n");
				return (void *)errn_code;
			}

			printf("[Fabryka][stanowisko 2][INFO] Wyworzono czekoladę typu 2!\n");
	}

	return (void *)0;
}

