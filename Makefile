COMPILER = clang
CFLAGS = -I./include -pthread -Wall -Wextra

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Searches for .c files in src/
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)

# Get everything from SRC_FILES that looks like src/%.c and change it to obj/%.o
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))
TARGET = $(BIN_DIR)/fabryka

all: $(TARGET)

# "|" means "make sure that whatever is to the right exists before doing left"
$(TARGET): $(OBJ_FILES) | $(BIN_DIR)
	# $^ - all prerequisites (all object files)
	# $@ - target name (bin/fabryka)
	$(COMPILER) $(CFLAGS) $^ -o $@

# Compile all .c files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	# $< - the first prerequisite (currently processed .c file)
	# -c option tells the compiler to not link, just compile
	$(COMPILER) $(CFLAGS) -c $< -o $@


$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)


.PHONY: all clean
