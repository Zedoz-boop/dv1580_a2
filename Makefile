# Compiler and Linking Variables
CC = gcc
CFLAGS = -Wall -fPIC
LIB_NAME = libmemory_manager.so

# Source and Object Files
SRC = memory_manager.c
OBJ = $(SRC:.c=.o)

# Default target
all: mmanager list test_mmanager test_list

# Rule to create the dynamic library for memory manager
mmanager: $(LIB_NAME)

$(LIB_NAME): $(OBJ)
	$(CC) -shared -o $@ $(OBJ)

# Rule to compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile linked list application and link with memory manager library
list: $(LIB_NAME) linked_list.o
	$(CC) -o linked_list_app linked_list.c test_linked_list.c -L. -lmemory_manager -lpthread -lm -Wl,-rpath=.

# Test memory manager program
test_mmanager: $(LIB_NAME)
	$(CC) -o test_memory_manager test_memory_manager.c -L. -lmemory_manager -lpthread -lm -Wl,-rpath=.

# Test linked list program
test_list: list

# Run all tests
run_tests: run_test_mmanager run_test_list

# Run memory manager test
run_test_mmanager: test_mmanager
	./test_memory_manager

# Run linked list test
run_test_list: test_list
	./linked_list_app

# Clean target to remove build files
clean:
	rm -f $(OBJ) $(LIB_NAME) test_memory_manager linked_list_app linked_list.o test_memory_manager.o test_linked_list.o
