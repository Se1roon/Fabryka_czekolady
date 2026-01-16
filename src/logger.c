#include "../include/common.h"

int main() {
    key_t key = ftok(".", IPC_KEY_ID);
    int msg_id = msgget(key, IPC_CREAT | 0600);
    if (msg_id == -1) {
        perror("Logger msgget");
        return 1;
    }

    FILE *f = fopen(LOG_FILE, "w");
    if (!f) {
        perror("Logger fopen");
        return 1;
    }

    printf("[Logger] Started. Writing to %s\n", LOG_FILE);
    fprintf(f, "--- SIMULATION START ---\n");

    LogMsg msg;
    while (1) {
        // Block until a log arrives (Type 2)
        if (msgrcv(msg_id, &msg, sizeof(msg.text), MSG_LOG, 0) == -1) {
            if (errno == EINTR)
                continue;
            break; // Queue removed
        }

        // Check for Poison Pill
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
