OBJS = callbacks.o vector.o queue.o document.o editor.o main.o
CC = clang 
FLAGS = -lncurses -Wall -Wextra -Werror -Wno-error=unused-parameter
LIBS_DIR = ../libs

editor: $(OBJS)
	$(CC) $(FLAGS) $(OBJS) -o editor
	rm -f *.o

callbacks.o: ../libs/callbacks.c
	$(CC) -c ../libs/callbacks.c

vector.o: src/vector.c
	$(CC) -c src/vector.c

queue.o: ../libs/queue.c
	$(CC) -c ../libs/queue.c

document.o: src/document.c
	$(CC) -c src/document.c

editor.o: src/editor.c
	$(CC) -c src/editor.c

main.o: src/main.c
	$(CC) -c src/main.c
