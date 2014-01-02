CC=gcc
CFLAGS=-pthread -ggdb -Wall
all:
	mkdir build -p
	$(CC) $(CFLAGS) source/webserver.c source/evilnetlib.c -o build/evilwebserver >& build/build.log
	cp html build/ -r
run:
	./build/evilwebserver
clean:
	rm -rf build