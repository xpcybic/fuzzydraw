#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <linux/limits.h>
#include "gd.h"

#define DEBUG 1

#define ERR_NO_IMG_ARG 1
#define ERR_FILE_HANDLE 2
#define ERR_MEM 255

//red/green/blue components of a color
#define RCOMP(c) ((c >> 16) & 0Xff)
#define GCOMP(c) ((c >> 8) & 0Xff)
#define BCOMP(c) ((c >> 0) & 0Xff)

#define ITERS_DEFAULT 1000
//#define ITERS_DEFAULT (gdImageSX(img) * gdImageSY(img))

//gdRect (x y width height)

//original image file (don't apply edits to it)
FILE *imgfilein = NULL;
//final output image file (create it at the end)
FILE *imgfileout = NULL;
//data for input image
gdImagePtr img = NULL;
//constructed image palette
int *pal = NULL;
//working images
gdImagePtr imgout1 = NULL;
gdImagePtr imgout2 = NULL;
//dimensions of area in which to draw shapes and do color comparisons
//int drawrectX, drawrectY, drawrectW, drawrectH

void cleanup()
{
	if (pal != NULL)
		free(pal);
	if (imgfilein != NULL)
		fclose(imgfilein);
	if (img != NULL)
		gdImageDestroy(img);
	if (imgout1 != NULL)
		gdImageDestroy(imgout1);
	if (imgout2 != NULL)
		gdImageDestroy(imgout2);
}

//get a list of all colors used in the image (in a bad slow way)
//returns: number of colors found in image
int getpalettelist(gdImagePtr img, int *pal)
{
	int palsz = 0;
	int px, i;
	for (int x = 0; x < gdImageSX(img); x++)
	{
		for (int y = 0; y < gdImageSY(img); y++)
		{
			px = gdImageTrueColorPixel(img, x, y);
			i = 0;
			while (i < palsz && pal[i] != px)
				i++;
			if (i == palsz)
				pal[palsz++] = px;
		}
	}
	return palsz;
}

//list palette count and a portion of colors in the palette (for debugging)
void showpalette(int *pal, size_t palsz)
{
	int palmax = 100;
	if (palsz < palmax)
		palmax = palsz;
	printf("Colors in image: %zd\n", palsz);
	for (int i = 0; i < palmax; i++)
		printf("%08x ", pal[i]);
	if (palsz > palmax)
		printf("\n(list truncated)");
	printf("\n");
}

//given some coordinates, pick a color and draw a shape onto the specified canvas.
//shapefunc can be one of the following:
//gdImageFilledEllipse(img, mx, my, w, h, c)
//gdImageFilledRectangle(img, x1, y1, x2, y2, color)
//gdImageLine(img, x1, y1, x2, y2, color)
void drawshape(void *shapefunc(gdImagePtr, int, int, int, int, int), gdImagePtr *img, int x1, int y1, int x2, int y2)
{

}

//squared euclidean distance between two ints (colors)
int dist(int a, int b)
{
	//color component differences
	char rd = RCOMP(a) - RCOMP(b);
	char gd = GCOMP(a) - GCOMP(b);
	char bd = BCOMP(a) - BCOMP(b);
	return (rd*rd + gd*gd + bd*bd);
}

//cumulative squared euclidean distance between two rectangular image regions
int regiondist(gdImagePtr img1, gdImagePtr img2, int subx, int suby, int subw, int subh)
{
	int distsum = 0;
	for (int x = subx; x < subx + subw; x++)
	{
		for (int y = suby; y < suby + subh; y++)
		{
			distsum += dist(gdImageGetTrueColorPixel(img1, x, y),
							gdImageGetTrueColorPixel(img2, x, y));
		}
	}
	return distsum;
}

//copy subregion of src to dest
void imgCopy(gdImagePtr src, gdImagePtr dest, int subx, int suby, int subw, int subh)
{
	gdImageCopy(dest, src, subx, suby, subx, suby, subw, subh);
}

int main(int argc, char *argv[])
{
	atexit(cleanup);

	srand(time(NULL));
	time_t start, end;
	//input/output filenames
	char *imgfileinname, *imgfileoutname;
	imgfileoutname = malloc(NAME_MAX);
	//size_t palsz = 0;
	//number of iterations i.e. how many objects to draw
	int iters;

	//arg handling
	if (argc < 2)
	{
		fprintf(stderr, "Please supply an image filename.\n");
		exit(ERR_NO_IMG_ARG);
	}

	//get filename and open file
	imgfileinname = argv[1];
	//char *ftype = strrchr(imgfileinname, '.');
	snprintf(imgfileoutname, NAME_MAX, "gdout-%d.png", (unsigned)time(NULL));
	imgfilein = fopen(imgfileinname, "rb");
	if (imgfilein == NULL)
	{
		fprintf(stderr, "Error opening input file %s: %s\n", imgfileinname, strerror(errno));
		exit(ERR_FILE_HANDLE);
	}

	//get input image and create outputs
	printf("Reading %s...\n", imgfileinname);
	img = gdImageCreateFromPng(imgfilein);
	if (img == NULL)
	{
		fprintf(stderr, "Out of memory\n");
		exit(ERR_MEM);
	}

	iters = ITERS_DEFAULT;
	printf("Defaulting to %d iterations.\n", iters);

	imgout1 = gdImageCreateTrueColor(gdImageSX(img), gdImageSY(img));
	imgout2 = gdImageCreateTrueColor(gdImageSX(img), gdImageSY(img));
	if (imgout1 == NULL || imgout2 == NULL)
	{
		fprintf(stderr, "Out of memory\n");
		exit(ERR_MEM);
	}

	//palsz = getpalettelist(img, pal);
	//pal = realloc(pal, palsz);
	//showpalette(pal, palsz);

	int color = 0x00000000;
	int i, x, y, x1, y1, r, w, h;

	start = clock();
	for (i = 0; i < iters; i++)
	{
		//get a color at a random coord
		x = rand() % gdImageSX(img);
		y = rand() % gdImageSY(img);
		color = gdImageGetTrueColorPixel(img, x, y);

		//paint a circle of that color at another random coord
		x = rand() % gdImageSX(img);
		y = rand() % gdImageSY(img);
		r = rand() % 35 + 5;
		gdImageFilledEllipse(imgout1, x, y, r, r, color);

		//compare current and previous to original and keep the closer of the two
		x1 = (x-r);
		y1 = (y-r);
		w = (r*2);
		h = r;
		if (x1<0) x1 = 0;
		if (y1<0) y1 = 0;
		if ((x1 + w) > gdImageSX(img)) w = (gdImageSX(img) - x1);
		if ((y1 + h) > gdImageSX(img)) h = (gdImageSX(img) - y1);
		if (regiondist(imgout1, img, x1, y1, (r*2), (r*2)) < regiondist(imgout2, img, x1, y1, (r*2), (r*2)))
			imgCopy(imgout1, imgout2, x1, y1, (r*2), (r*2));
		else
			imgCopy(imgout2, imgout1, x1, y1, (r*2), (r*2));
	}

	//write final image to file
#if DEBUG
	imgfileoutname = "(temp)";
#else
	imgfileout = fopen(imgfileoutname, "wb");
	if (imgfileout == NULL)
	{
		fprintf(stderr, "Error opening output file %s: %s\n", imgfileoutname, strerror(errno));
		exit(ERR_FILE_HANDLE);
	}
	gdImagePng(imgout1, imgfileout);
#endif

	end = clock();
	printf("Finished drawing %s in %ldms.\n", imgfileoutname, (end - start) * 1000 / CLOCKS_PER_SEC);

	/* bad palette reading code here
	//preallocate for palette storage - this is very excessive and could be done dynamically
	pal = calloc((gdImageSX(img) * gdImageSY(img)), sizeof(int));
	*/

	printf("Done.\n");
	return 0;
}
