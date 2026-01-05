#ifndef DYREKTOR_H
#define DYREKTOR_H

void clean_up(int sem_id, int shm_id);
void* handle_user_interface(void* child_pids);
void* sig_handler(int sig_num);

#endif