CC=gcc
CFLAGS=-g -Wall -O2 -lgd -lpng -ljpeg -lz -ljpeg -lm

gdimgdraw: gdimgdraw.c
	$(CC) -o $@ $? $(CFLAGS)

gdtest: gdtest.c
	$(CC) -o $@ $? $(CFLAGS)
