#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "gd.h"

//original image file (don't apply edits to it)
FILE *imgfilein = NULL;
FILE *imgfileout = NULL;
gdImagePtr img = NULL;
gdImagePtr imgplt = NULL;
gdImagePtr imgout1 = NULL;
gdImagePtr imgout2 = NULL;

void cleanup()
{
	if (imgfilein != NULL)
		fclose(imgfilein);
	/*
	if (img != NULL)
		gdImageDestroy(img);
	if (imgplt != NULL)
		gdImageDestroy(imgplt);
	if (imgout1 != NULL)
		gdImageDestroy(imgout1);
	if (imgout2 != NULL)
		gdImageDestroy(imgout2);
	*/
}

int main(int argc, char *argv[])
{
	atexit(cleanup);

	char *imgfileinname = NULL;
	char *imgfileoutname = NULL;

	//arg handling
	if (argc < 2)
	{
		fprintf(stderr, "Please supply an image filename.\n");
		exit(1);
	}

	//get filename and open file
	imgfileinname = argv[1];
	imgfileoutname = "gdout.png";	//TODO: copy input file format?
	imgfilein = fopen(imgfileinname, "rb");
	if (imgfilein == NULL)
	{
		fprintf(stderr, "Error opening %s: %s\n", imgfileinname, strerror(errno));
		exit(2);
	}

	printf("Reading %s...\n", imgfileinname);
	printf("Output filename will be %s.\n", imgfileoutname);

	return 0;
}
