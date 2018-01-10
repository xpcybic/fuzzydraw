CC=gcc
CFLAGS=-g -Wall -O2 -lgd -lpng -ljpeg -lz -lm

gdimgdraw: gdimgdraw.c
	$(CC) -o $@ $? $(CFLAGS)

debug: gdimgdraw.c
	$(CC) -o gdimgdraw $? $(CFLAGS) -DDEBUG -g

.PHONY: clean
clean:
	rm -f gdimgdraw
