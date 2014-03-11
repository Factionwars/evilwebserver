CC=gcc
CFLAGS=-pthread -ggdb -O -Wall -Wextra -lm -std=c11 -D_GNU_SOURCE  -I./source/libraries/objectivity/.  $(OPTFLAGS)
libs=ldl $(OPTLIBS)

SOURCES=$(wildcard source/**/*.c source/*.c source/libraries/cJSON/*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

TARGET=build/evilwebserver 
SO_TARGET=$(patsubst %.s,%,$(TARGET))

all: $(TARGET) $(SO_TARGET)
	@echo "Happy serving -- Factionwars@evilzone.org"
$(TARGET): build objectivity $(OBJECTS)

$(SO_TARGET): $(TARGET) $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(wildcard source/libraries/**/*.a)
clean:
	rm -rf build $(OBJECTS)
	$(MAKE) clean -C source/libraries/objectivity/
build:
	@mkdir -p build
objectivity:
	$(MAKE) -C source/libraries/objectivity/
test:
	$(MAKE)
	valgrind --track-origins=yes --leak-check=full --track-fds=yes build/evilwebserver