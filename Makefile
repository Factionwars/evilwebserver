CC=gcc
CFLAGS=-pthread -ggdb
all:
	$(CC) $(CFLAGS) webserver.c -o build/evilwebserver >& build.log
clean:
	rm -rf build/*
