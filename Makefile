CC=gcc
CFLAGS=-pthread -ggdb -O -Wall -Wextra 
all:
	@mkdir build -p
	@echo Building jsmn Json Library
	@make -C source/jsmn > build/build.log 2>&1
	@cd ../../
	@echo Building EvilWebserver

	@$(CC) $(CFLAGS) source/webserver.c source/evilnetlib.c -o build/evilwebserver > build/build.log 2>&1
	@cp scripts build/ -r
	@echo Done! Output written to build/build.log
clean:
	@echo Removing build folder
	@rm -rf build
	@echo Done cleaning up!