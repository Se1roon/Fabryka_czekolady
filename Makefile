CC = clang
CFLAGS = -Wall -Wextra

run: bin/dyrektor bin/logger bin/dostawcy bin/fabryka 

bin/logger: src/logger.c include/common.h
	$(CC) src/logger.c -o $@ $(CFLAGS)

bin/dostawcy: src/dostawcy.c include/common.h
	$(CC) src/dostawcy.c -o $@ $(CFLAGS)

bin/fabryka: src/fabryka.c include/common.h
	$(CC) src/fabryka.c -o $@ $(CFLAGS)

bin/dyrektor: src/dyrektor.c include/common.h
	$(CC) src/dyrektor.c -o $@ $(CFLAGS)

directories:
	mkdir bin

clean:
	rm -rf bin/
	rm magazine.bin
	rm simulation.log
