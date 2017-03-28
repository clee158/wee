OBJS = client.c .base64.c
CC = gcc
FLAGS = -Wall -pthread

client : $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o client 

clean:
	rm -rf client
