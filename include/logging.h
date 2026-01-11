#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>

#define LOG_FILE            "./.fc.log"
#define PRODUCTION_LOG_RATE 200
#define LOG_MESSAGE_MAX_LEN 256

typedef struct {
    long mtype;
    char message[LOG_MESSAGE_MAX_LEN];
} LogMessage;

#endif
