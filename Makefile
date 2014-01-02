CC=gcc
CFLAGS=-pthread -ggdb
all:
	$(CC) $(CFLAGS) main.c -o build/evilwebserver >& build.log
clean:
	rm -rf build/*
