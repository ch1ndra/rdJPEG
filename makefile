CC=gcc
CFLAGS=-Wall -Wextra -O2

all: jpglib jpg2bmp

jpglib: 
	$(CC) $(CFLAGS) -c jpg.c -o jpg.o
	
jpg2bmp: jpglib
	$(CC) $(CFLAGS) jpg2bmp.c jpg.o -o jpg2bmp

clean:
	rm *.o
