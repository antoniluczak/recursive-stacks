CC = gcc
CFLAGS = -Wall -Wextra -Wno-implicit-fallthrough -std=gnu23 -fPIC -O2
LDFLAGS = -shared \
	-Wl,--wrap=malloc \
	-Wl,--wrap=calloc \
	-Wl,--wrap=realloc \
	-Wl,--wrap=reallocarray \
	-Wl,--wrap=free \
	-Wl,--wrap=strdup \
	-Wl,--wrap=strndup

TARGET = librstack.so

.PHONY: all clean example

all: $(TARGET)

librstack.so: rstack.o memory_tests.o
	$(CC) $(LDFLAGS) -o $@ $^

rstack.o: rstack.c rstack.h memory_tests.h
	$(CC) $(CFLAGS) -c rstack.c -o $@

memory_tests.o: memory_tests.c memory_tests.h
	$(CC) $(CFLAGS) -c memory_tests.c -o $@

example: $(TARGET) rstack_example.c
	$(CC) $(CFLAGS) rstack_example.c -L. -lrstack -Wl,-rpath=. -o rstack_example

clean:
	rm -f *.o $(TARGET) rstack_example

