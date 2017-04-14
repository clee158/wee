OBJS_DIR = obj
SRC_DIR = src

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJS_DIR)/%.o, $(SRC))
EXE = weeditor

CC = clang
WARNINGS = -Wall -Wextra -Werror -Wno-error=unused-parameter
CFLAGS = $(WARNINGS) -g -c 
LD = clang
LDFLAGS = -lncurses

.PHONY: all
all: $(EXE)

.PHONY: clean
clean:
	rm -f $(OBJ)
	rm -f $(EXE)

$(EXE): $(OBJS_DIR) $(OBJ)
	$(LD) $(OBJ) $(LDFLAGS) -o $@

$(OBJS_DIR):
	mkdir $@

$(OBJS_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJS_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h
	$(CC) $(CFLAGS) $< -o $@
