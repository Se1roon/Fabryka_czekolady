#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#include "../include/logging.h"

static void *sig_handler(int sig_num);

static FILE *log_file = NULL;


int main(void) {
    // Handle signals
    struct sigaction sig_action;
    sig_action.sa_handler = (void *)sig_handler;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sig_action, NULL) != 0) {
    }

    // Obtain IPC key
    key_t ipc_key = ftok(".", 69);
    if (ipc_key == -1) {
        fprintf(stderr, "[Logging][ERRN] Unable to create IPC Key! (%s)\n", strerror(errno));
        return 1;
    }

    // Join Message Queue
    int msg_id = msgget(ipc_key, IPC_CREAT | 0600);
    if (msg_id == -1) {
        fprintf(stderr, "[Logging][ERRN] Unable to join the Message Queue! (%s)\n", strerror(errno));
        return -1;
    }

    log_file = fopen(LOG_FILE, "w");
    if (!log_file) {
        fprintf(stderr, "[Logging][ERRN] Failed to open log file! (%s)\n", strerror(errno));
        return -1;
    }

    LogMessage log_msg;
    while (true) {
        if (msgrcv(msg_id, &log_msg, sizeof(log_msg.message), 0, 0) == -1)
            break;

        fprintf(log_file, "%s\n", log_msg.message);
    }

    fclose(log_file);
    return 0;
}

void *sig_handler(int sig_num) {
    if (sig_num == SIGINT) {
        fclose(log_file);
        exit(0);
    }

    return NULL;
}
