CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -Iinclude -MMD -MP
LIBS    = -lm

SRC     = $(wildcard src/*.c)
OBJ     = $(SRC:.c=.o)
TARGET  = placer

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $^ -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	mkdir -p output
	./$(TARGET)

clean:
	rm -f src/*.o src/*.d $(TARGET)

-include $(OBJ:.o=.d)

.PHONY: all run clean
