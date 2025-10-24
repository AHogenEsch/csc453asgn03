# Compiler and Flags
CC = gcc
CFLAGS = -Wall
# LDLIBS: Libraries required for linking
# -lpthread: POSIX Threads library 
# -lrt: Realtime library (for dawdle/nanosleep)
LDLIBS = -lpthread -lrt

# Source, Object, and Target Files
SRCS = dine.c
OBJS = $(SRCS:.c=.o)
TARGET = dine 

.PHONY: all clean

# The default target: builds the 'dine' executable
all: $(TARGET)

# Rule to link the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDLIBS)

# Rule to compile C source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to clean up generated files
clean:
	rm -f $(OBJS) $(TARGET)