CC=gcc
CFLAGS=-pthread -ggdb -Wall
all:
	@echo Building EvilWebserver
	@mkdir build -p
	@$(CC) $(CFLAGS) source/webserver.c source/evilnetlib.c -o build/evilwebserver >& build/build.log
	@cp html build/ -r
	@echo Done! Output written to build/build.log
clean:
	@echo Removing build folder
	@rm -rf build
	@echo Done cleaning up!