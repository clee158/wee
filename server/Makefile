OBJS = server.c
CC = gcc
FLAGS = -Wall -pthread -g

server : $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o server

clean:
	rm -rf server
