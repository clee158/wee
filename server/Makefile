OBJS = callbacks.o queue.o server.o
CC = gcc
FLAGS = -Wall -pthread -g
LIBS_DIR = ../libs

server : $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o server
	rm -f *.o

server.o: server.c
	$(CC) -c server.c

queue.o: $(LIBS_DIR)/queue.c
	$(CC) -c ../libs/queue.c

callbacks.o: $(LIBS_DIR)/callbacks.c
	$(CC) -c ../libs/callbacks.c

clean:
	rm -f server
