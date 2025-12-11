#ifndef LOGGING_H
#define LOGGING_H

#define LOG_FILE "./.fc.log"

int init_log();
int write_log(int fd, const char *format, ...);

#endif
