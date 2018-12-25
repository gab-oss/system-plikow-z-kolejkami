CC = gcc
CFLAGS = -Wall -g

SRCS = fs.c
OBJS = $(SRCS:.c=.o)

all: fs.o

clean:
	rm -rf build

fs.o: fs.c fs.h
	$(CC) $(CFLAGS) -c -o build/fs.o fs.c
