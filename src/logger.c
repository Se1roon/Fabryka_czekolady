#include "../include/common.h"

int main() {
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

    printf("[Logger] Started. Writing to %s\n", LOG_FILE);
    fprintf(f, "--- SIMULATION START ---\n");

    LogMsg msg;
    while (1) {
        // Block until a log arrives (Type 2)
        if (msgrcv(msg_id, &msg, sizeof(msg.text), MSG_LOG, 0) == -1) {
            if (errno == EINTR)
                continue;
            break;
        }

        if (strcmp(msg.text, "TERMINATE") == 0)
            break;

        // Add Timestamp
        time_t now = time(NULL);
        char *t = ctime(&now);
        t[strlen(t) - 1] = '\0'; // Trim newline

        fprintf(f, "[%s] %s\n", t, msg.text);
        fflush(f); // Ensure write to disk
    }

    fprintf(f, "--- SIMULATION END ---\n");
    fclose(f);
    return 0;
}
