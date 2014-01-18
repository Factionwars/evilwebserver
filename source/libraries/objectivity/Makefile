# You can put your build options here
CFLAGS=-m32

all: objectivity.a 

objectivity.a: object.o list.o
	$(AR) rc $@ $^

object.o: object.c object.h
	$(CC) -c $(CFLAGS) $< -o $@

list.o: list.c list.h
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f object.o list.o
	rm -f objectivity.a


