CC=gcc
CFLAGS=-Wall -O2 -lgd -lpng -lz -ljpeg -lm

gdtest: gdtest.c
	$(CC) -o $@ $? $(CFLAGS)
