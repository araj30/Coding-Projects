CC = clang
CFLAGS = -Wall -Wpedantic -Werror -Wextra

all: cacher

cacher: cacher.o
	$(CC) -o cacher cacher.o 

cacher.o: cacher.c
	$(CC) $(CFLAGS) -c cacher.c

clean:
	rm -f cacher cacher.o 

format:
	clang-format -i -style=file *.[ch]
