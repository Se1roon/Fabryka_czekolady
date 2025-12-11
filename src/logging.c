#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "logging.h"


int init_log() {
	int fd = open(LOG_FILE, O_RDWR | O_CREAT | O_APPEND, 0644);
	if (fd < 0) {
		fprintf(stderr, "[LOG][ERRN] Unable to open log file '%s'\n", LOG_FILE);
		return -1;
	}
	
	return fd;
}

int write_log(int fd, const char *format, ...) {
	char buffer[2048];

	va_list args;
	va_start(args, format);

	int len = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	if (len < 0) {
		fprintf(stderr, "[LOG][ERRN] Something went wrong with formating log message!\n");
		return 2;
	}

	if (write(fd, buffer, len) < 0) {
		fprintf(stderr, "[LOG][ERRN] Unable to write to log file '%s'\n", LOG_FILE);
		return 1;
	}

	return 0;
}

