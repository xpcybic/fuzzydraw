CC=gcc
CFLAGS=-g -Wall -O2 -lgd -lpng -ljpeg -lz -lm

fuzzydraw: fuzzydraw.c
	$(CC) -o $@ $? $(CFLAGS)

debug: fuzzydraw.c
	$(CC) -o fuzzydraw $? $(CFLAGS) -DDEBUG -g -pg -no-pie -fPIC

.PHONY: clean
clean:
	rm -f fuzzydraw gmon.out
