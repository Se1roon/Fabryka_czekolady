CC = clang
CFLAGS = -Wall -Wextra

run: bin/dyrektor bin/logger bin/dostawcy bin/fabryka

bin/logger: src/logger.c
	$(CC) $^ -o $@ $(CFLAGS)

bin/dostawcy: src/dostawcy.c
	$(CC) $^ -o $@ $(CFLAGS)

bin/fabryka: src/fabryka.c
	$(CC) $^ -o $@ $(CFLAGS)

bin/dyrektor: src/dyrektor.c
	$(CC) $^ -o $@ $(CFLAGS)

directories:
	mkdir bin

clean:
	rm -rf bin/
	rm magazine.bin
	rm simulation.log
