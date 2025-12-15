#ifndef DOSTAWCY_H
#define DOSTAWCY_H

#include <pthread.h>

#include "common.h"

#define DELIVERY_GUYS_COUNT 4

/* type:
 *	1 - A
 *	2 - B
 *	3 - C
 *	4 - D
 */
typedef struct {
    pthread_t tid;
    int type;
    int sem_id;
    SHM_DATA* magazine_data;
} delivery_t;

void* delivery(void* delivery_data);

#endif
