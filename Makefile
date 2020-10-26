CC = gcc
RM = rm

all: glass

glass: glass.c
	$(CC) -o glass glass.c

debug: glass.c
	$(CC) -D DEBUG -g -o glass glass.c

clean:
	$(RM) glass
