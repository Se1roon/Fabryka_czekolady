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
#define X_TYPE_TO_PRODUCE 125000
#define Y_TYPE_TO_PRODUCE 250000

// Keep this < 50000 so that semaphore operations don't fail
// Should be able to handle up to around 75,000 on most systems, but there is no reason to make it that big.
// REMEMBER TO DELETE magazine.bin AFTER CHANGING THIS
#define MAGAZINE_CAPACITY 1000

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

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

typedef struct {
    char buffer[MAGAZINE_CAPACITY];
    int typeX_produced;
    int typeY_produced;
} Magazine;

// Colors
#define ERROR_CLR_SET   "\x1b[31m"
#define INFO_CLR_SET    "\x1b[32m"
#define WARNING_CLR_SET "\x1b[33m"
#define CLR_RST         "\x1b[0m"

// Semaphore ID's
// Empty - slots available
// Full - produced
#define SEM_MAGAZINE 0
#define SEM_EMPTY_A  1
#define SEM_EMPTY_B  2
#define SEM_EMPTY_C  3
#define SEM_EMPTY_D  4
#define SEM_FULL_A   5
#define SEM_FULL_B   6
#define SEM_FULL_C   7
#define SEM_FULL_D   8

static int get_semaphore_id(char type, bool is_empty) {
    int base = (is_empty) ? SEM_EMPTY_A : SEM_FULL_A;
    if (type == 'A')
        return base;
    if (type == 'B')
        return base + 1;
    if (type == 'C')
        return base + 2;
    if (type == 'D')
        return base + 3;

    return -1;
}

#define MSG_LOG 1

typedef struct {
    long mtype;
    char text[256];
} LogMsg;

static void send_log(int msg_id, const char *fmt, ...) {
    LogMsg msg;
    msg.mtype = MSG_LOG;

    va_list args;
    va_start(args, fmt);
    vsnprintf(msg.text, sizeof(msg.text), fmt, args);
    va_end(args);

    msgsnd(msg_id, &msg, sizeof(msg.text), 0);
}

#endif
