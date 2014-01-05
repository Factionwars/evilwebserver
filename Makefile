CC=gcc
CFLAGS=-pthread -ggdb -O -Wall -Wextra
all:
	@echo Building EvilWebserver
	@mkdir build -p
	@$(CC) $(CFLAGS) source/webserver.c source/evilnetlib.c -o build/evilwebserver > build/build.log 2>&1
	@cp html build/ -r
	@echo Done! Output written to build/build.log
clean:
	@echo Removing build folder
	@rm -rf build
	@echo Done cleaning up!