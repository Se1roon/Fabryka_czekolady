#ifndef FABRYKA_H
#define FABRYKA_H

#include <stddef.h>

#include "../include/common.h"

#define SAVE_FILE "./.magazyn"

typedef struct {
    int sem_id;
    SHM_DATA* magazine_data;
} station_t;

void* station_1(void* lock);
void* station_2(void* lock);

int restore_state();
int save_state();

static void* sig_handler(int sig_num);

#endif
