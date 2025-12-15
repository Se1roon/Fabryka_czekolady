#ifndef DOSTAWCY_H
#define DOSTAWCY_H

#include <pthread.h>
#include <stdbool.h>

#include "common.h"

#define DELIVERY_GUYS_COUNT 4

typedef struct {
    pthread_t tid;
    component_type type;
    int sem_id;
    SHM_DATA* magazine_data;
} delivery_t;

void* delivery(void* delivery_data);
bool magazine_available(SHM_DATA* magazine);

#endif
