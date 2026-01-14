CC = clang
CFLAGS = -Wall -Wextra

run: directories bin/dyrektor bin/logging bin/dostawcy bin/fabryka

bin/logging: src/logging.c
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
	rm .magazyn
	rm .fc.log
