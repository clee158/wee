OBJS = callbacks.o vector.o queue.o document.o editor.o main.o .base64.o
CC = clang 
FLAGS = -lncurses -g -DDEBUG -pthread -Wall -Wextra -Werror -Wno-error=unused-parameter
LIBS_DIR = ../libs

editor: $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o editor
	rm -f .*.o
	rm -f *.o

callbacks.o: ../libs/callbacks.c
	$(CC) -c ../libs/callbacks.c

vector.o: vector.c
	$(CC) -c vector.c

queue.o: ../libs/queue.c
	$(CC) -c ../libs/queue.c

document.o: document.c
	$(CC) -c document.c

editor.o: editor.c
	$(CC) -c editor.c

main.o: main.c
	$(CC) -c main.c

.base64.o: .base64.c
	$(CC) -c .base64.c

clean:
	rm -f editor
	rm -f log.txt
