CC = clang
CFLAGS = -Wall -Wpedantic -Werror -Wextra
LFLAGS = -lm

all: banhammer

banhammer: banhammer.o speck.o ht.o bst.o node.o bf.o bv.o parser.o
	$(CC) $(CLFAGS) banhammer.o speck.o ht.o bst.o node.o bf.o bv.o parser.o -o banhammer $(LFLAGS)

banhammer.o: banhammer.c messages.h salts.h
	$(CC) $(CFLAGS) -c banhammer.c

speck.o: speck.c speck.h
	$(CC) $(CFLAGS) -c speck.c

ht.o: ht.c ht.h
	$(CC) $(CFLAGS) -c ht.c

bst.o: bst.c bst.h
	$(CC) $(CFLAGS) -c bst.c

parser.o: parser.c parser.h
	$(CC) $(CFLAGS) -c parser.c

bf.o: bf.c bf.h
	$(CC) $(CFLAGS) -c bf.c

bv.o: bv.c bv.h
	$(CC) $(CFLAGS) -c bv.c

node.o: node.c node.h
	$(CC) $(CFLAGS) -c node.c

clean:
	rm -f banhammer *.o

grind: all
	valgrind --leak-check=full ./banhammer

format:
	clang-format -i -style=file *.{c,h}

add: clean format
	git add *.c
	git status
