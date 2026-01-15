#ifndef DOSTAWCY_H
#define DOSTAWCY_H

#include <pthread.h>
#include <stdbool.h>

#include "magazine.h"

#define DELIVERIES_COUNT 4

typedef struct {
    pthread_t tid;
    E_Component type;
    bool is_working;
    int sem_id;
    int msg_id;
    Magazine *magazine;
} Delivery;

void *delivery_start(void *delivery_data);

#endif
