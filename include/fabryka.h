#ifndef FABRYKA_H
#define FABRYKA_H

#include <stddef.h>

#include "common.h"

typedef struct {
    int sem_id;
    int msg_id;
    SHM_DATA *magazine_data;
} station_t;

void *station_1(void *lock);
void *station_2(void *lock);

void write_log(int msg_id, char *message);

#endif
