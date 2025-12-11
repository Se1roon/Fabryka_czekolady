#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main(void) {
	pid_t cpid;

	cpid = fork();
	switch (cpid) {
		case -1:
			fprintf(stderr, "[Dyrektor][main][ERRN] Failed to create child process!\n");
			exit(1);
		case 0: // Child
			if (execl("./bin/fabryka", "./bin/fabryka", NULL) < 0) {
				fprintf(stderr, "[Dyrektor][main][ERRN] Faield to create Fabryka process!\n");
				exit(1);
			}
			break;
		default: // Parent
			printf("Parent\n");
	};
	
	return 0;
}

