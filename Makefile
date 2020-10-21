CC = gcc
RM = rm

all: glass

glass: glass.c
	$(CC) -o glass glass.c

debug: glass.c
	$(CC) -o glass -g glass.c

clean:
	$(RM) glass
