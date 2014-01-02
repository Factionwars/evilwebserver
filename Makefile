CC=gcc
CFLAGS=-pthread -ggdb
all:
	mkdir build
	$(CC) $(CFLAGS) webserver.c -o build/evilwebserver >& build.log
clean:
	rm -rf build
