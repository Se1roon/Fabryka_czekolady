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

#define MAGAZINE_CAPACITY         1000
#define SLOTS_LIMIT               (MAGAZINE_CAPACITY / 4)
#define SMALL_COMPONENTS_TRESHOLD 20

// Components size
#define GET_SIZE(c) ((c) == 'C' ? 2 : ((c) == 'D' ? 3 : 1))

// Maps 'A' -> 0, 'B' -> 1, ... in component_counts
#define GET_TYPE_IDX(c) ((c) - 'A')

#define MSG_LOG 1

// Ring Shared Memory - Magazyn
typedef struct {
    int head; // Read index		(Fabryka)
    int tail; // Write index	(Dostawcy)
    int current_usage;

    int type1_produced;
    int type2_produced;

    /* [0] -> A
     * [1] -> B
     * [2] -> C
     * [3] -> D
     */
    int component_counts[4];

    char buffer[MAGAZINE_CAPACITY];
} Magazine;

// Message Queue structs
typedef struct {
    long mtype;
    int command;
} CmdMsg;

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
    msgsnd(msg_id, &msg, sizeof(msg.text), IPC_NOWAIT);
}

#endif
