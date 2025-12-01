#ifndef FABRYKA_H
#define FABRYKA_H

#include <stddef.h>

#define SAVE_FILE "./.magazyn"

typedef struct {
	size_t capacity;
	size_t a_count;
	size_t b_count;
	size_t c_count;
	size_t d_count;
} magazyn_t;

extern magazyn_t magazyn;

void *stanowisko_1(void *lock);
void *stanowisko_2(void *lock);

int restore_state();
int save_state();

static void sig_handler(int signo);

#endif
