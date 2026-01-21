#ifndef COMMON_H
#define COMMON_H

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define IPC_KEY_ID 7616

#define LOG_FILE "simulation.log"

// Size of the components in bytes
#define A_COMPONENT_SIZE 1
#define B_COMPONENT_SIZE 1
#define C_COMPONENT_SIZE 2
#define D_COMPONENT_SIZE 3

// X: A + B + C
// Y: A + B + D
#define X_TYPE_SIZE (A_COMPONENT_SIZE) + (B_COMPONENT_SIZE) + (C_COMPONENT_SIZE)
#define Y_TYPE_SIZE (A_COMPONENT_SIZE) + (B_COMPONENT_SIZE) + (D_COMPONENT_SIZE)

// Change these
#define X_TYPE_TO_PRODUCE 5000000
#define Y_TYPE_TO_PRODUCE 400

#define MAGAZINE_CAPACITY (X_TYPE_TO_PRODUCE) * (X_TYPE_SIZE) + (Y_TYPE_TO_PRODUCE) * (Y_TYPE_SIZE)

#define COMPONENT_SPACE (MAGAZINE_CAPACITY) / ((A_COMPONENT_SIZE) + (B_COMPONENT_SIZE) + (C_COMPONENT_SIZE) + (D_COMPONENT_SIZE))

// IsA - index start A
// IkA - index end A
#define IsA 0
#define IkA (COMPONENT_SPACE) - 1
#define IsB (IkA) + 1
#define IkB (IsB) + ((COMPONENT_SPACE) - 1)
#define IsC (IkB) + 1
#define IkC (IsC) + (2 * (COMPONENT_SPACE) - 1)
#define IsD (IkC) + 1
#define IkD (IsD) + (3 * (COMPONENT_SPACE) - 1)

// Colors
#define ERROR_CLR_SET   "\x1b[31m"
#define INFO_CLR_SET    "\x1b[32m"
#define WARNING_CLR_SET "\x1b[33m"
#define CLR_RST         "\x1b[0m"

#define MSG_LOG 1

typedef struct {
    char buffer[MAGAZINE_CAPACITY];

    int A_count;
    int B_count;
    int C_count;
    int D_count;

    int type1_produced;
    int type2_produced;
} Magazine;

typedef struct {
    long mtype;
    char text[256];
} LogMsg;

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

static void send_log(int msg_id, const char *fmt, ...) {
    LogMsg msg;
    msg.mtype = MSG_LOG;

    va_list args;
    va_start(args, fmt);
    vsnprintf(msg.text, sizeof(msg.text), fmt, args);
    va_end(args);

    // Use IPC_NOWAIT so workers don't freeze if logger is slow
    msgsnd(msg_id, &msg, sizeof(msg.text), 0);
}

#endif
