#include <stdbool.h>

#include "../include/common.h"

static bool terminated = false;

void log_msg(FILE *f, LogMsg *msg);

void signal_handler(int sig_num);

int main() {
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        fprintf(stderr, "[Logger] Failed to assign signal handler to SIGINT! (%s)\n", strerror(errno));
        return -1;
    }

    key_t ipc_key = ftok(".", IPC_KEY_ID);
    if (ipc_key == -1) {
        fprintf(stderr, "[Logger] Failed to create IPC key for IPC communication! (%s)\n", strerror(errno));
        return -1;
    }

    int msg_id = msgget(ipc_key, IPC_CREAT | 0600);
    if (msg_id == -1) {
        fprintf(stderr, "[Logger] Failed to join the Message Queue! (%s)\n", strerror(errno));
        return -1;
    }

    FILE *f = fopen(LOG_FILE, "w");
    if (!f) {
        fprintf(stderr, "[Logger] Failed to open %s file! (%s)\n", LOG_FILE, strerror(errno));
        return -1;
    }

    fprintf(f, "--- SIMULATION START ---\n");

    LogMsg msg;
    while (!terminated) {
        if (msgrcv(msg_id, &msg, sizeof(msg.text), MSG_LOG, 0) == -1) {
            if (errno == EINTR)
                continue;
            break;
        }

        // Add Timestamp
        log_msg(f, &msg);
    }

    while (true) {
        struct msqid_ds queue;
        msgctl(msg_id, IPC_STAT, &queue);
        if (queue.msg_qnum == 0)
            break;

        size_t bytes = msgrcv(msg_id, &msg, sizeof(msg.text), MSG_LOG, 0);

        if (bytes >= 0)
            log_msg(f, &msg);
        else
            break;
    }

    fprintf(f, "--- SIMULATION END ---\n");

    fclose(f);
    return 0;
}

void log_msg(FILE *f, LogMsg *msg) {
    time_t now = time(NULL);
    char *t = ctime(&now);
    t[strlen(t) - 1] = '\0'; // Trim newline

    fprintf(f, "[%s] %s\n", t, msg->text);
    fprintf(stdout, "[%s] %s\n", t, msg->text);
    fflush(f);
    fflush(stdout);
}

void signal_handler(int sig_num) {
    terminated = true;
}
