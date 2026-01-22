CC = clang
CFLAGS = -Wall -Wextra

run: bin/dyrektor bin/logger bin/dostawca bin/pracownik

bin/logger: src/logger.c include/common.h
	$(CC) src/logger.c -o $@ $(CFLAGS)

bin/dostawca: src/dostawca.c include/common.h
	$(CC) src/dostawca.c -o $@ $(CFLAGS)

bin/pracownik: src/pracownik.c include/common.h
	$(CC) src/pracownik.c -o $@ $(CFLAGS)

bin/dyrektor: src/dyrektor.c include/common.h
	$(CC) src/dyrektor.c -o $@ $(CFLAGS)

directories:
	mkdir bin

clean:
	rm -rf bin/
	rm magazine.bin
	rm simulation.log
