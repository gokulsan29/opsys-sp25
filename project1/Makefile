OUTPUT := dirwatch
INPUT := dirwatch.c

CC := gcc
CFLAGS := --std=c99 -Wall

.PHONY: all clean

all: $(OUTPUT)

$(OUTPUT): $(INPUT)
	$(CC) $(CFLAGS) $< -o $@

clean::
	rm $(OUTPUT)

