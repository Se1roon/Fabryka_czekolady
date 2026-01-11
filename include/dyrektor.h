#ifndef DYREKTOR_H
#define DYREKTOR_H

#include <sys/types.h>

#include "common.h"

typedef struct {
    pid_t *children;
    SHM_DATA *data;
} UI_data;

void clean_up(int sem_id, int shm_id, int msg_id);
void *handle_user_interface(void *ui_data);

int restore_state();
int save_state();

#endif
