CC = clang
CFLAGS = -Wall -Wextra

run: bin/dyrektor bin/logger bin/dostawca bin/pracownik bin/check_magazine

bin/logger: src/logger.c include/common.h
	$(CC) src/logger.c -o $@ $(CFLAGS)

bin/dostawca: src/dostawca.c include/common.h
	$(CC) src/dostawca.c -o $@ $(CFLAGS)

bin/pracownik: src/pracownik.c include/common.h
	$(CC) src/pracownik.c -o $@ $(CFLAGS)

bin/dyrektor: src/dyrektor.c include/common.h
	$(CC) src/dyrektor.c -o $@ $(CFLAGS)

bin/check_magazine: src/check_magazine.c include/common.h
	$(CC) src/check_magazine.c -o $@ $(CFLAGS)

directories:
	mkdir bin

clean:
	rm -rf bin/
	rm magazine.bin
	rm simulation.log
