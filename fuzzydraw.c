#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <linux/limits.h>
#include "gd.h"

#define ERR_ARGS 1
#define ERR_FILE_HANDLE 2
#define ERR_FTYPE 3
#define ERR_MEM 255

//red/green/blue components of a color
#define RCOMP(c) ((c >> 16) & 0Xff)
#define GCOMP(c) ((c >> 8) & 0Xff)
#define BCOMP(c) ((c >> 0) & 0Xff)

#define ITERS_DEFAULT (gdImageSX(imgin) * gdImageSY(imgin) / 2)
#define DIST_RES_DEFAULT 1

#define HELPTEXT \
	"Usage: fuzzydraw [options] <file>\n"						\
	"Options:\n"												\
	"-h: Print this help message\n"								\
	"-i: Number of iterations\n"								\
	"-m: Randomization mode:\n"									\
	"  0 Match colors from original image\n"					\
	"  1 Sample colors from original image\n"					\
	"  2 Use random colors\n"									\
	"-o: Output filename\n"										\
	"-q: Output image quality reduction (1-10; default 1)\n"

//original image file (don't apply edits to it)
FILE *imgfilein = NULL;
//final output image file (create it at the end)
FILE *imgfileout = NULL;
//data for input image
gdImagePtr imgin = NULL;
//working images
gdImagePtr imgout1 = NULL;
gdImagePtr imgout2 = NULL;
////constructed image palette
//int *pal = NULL;

void cleanup()
{
	//if (pal != NULL)
		//free(pal);
	if (imgfilein != NULL)
		fclose(imgfilein);
	if (imgin != NULL)
		gdImageDestroy(imgin);
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

/*given some coordinates, pick a color and draw a shape onto the specified canvas.
 * shapefunc can be one of the following:
 * gdImageFilledEllipse(img, mx, my, w, h, c)
 * gdImageFilledRectangle(img, x1, y1, x2, y2, color)
 * gdImageLine(img, x1, y1, x2, y2, color)
 * void drawshape(void *shapefunc(gdImagePtr, int, int, int, int, int), gdImagePtr *img, int x1, int y1, int x2, int y2)
 */
//{
//
//}

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
int regiondist(gdImagePtr img1, gdImagePtr img2, int subx, int suby, int subw, int subh, int distres)
{
	int distsum = 0;
	int subxr = subx + subw;
	int subyr = suby + subh;
	for (int x = subx; x < subxr; x += distres)
	{
		for (int y = suby; y < subyr; y += distres)
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
	char arg;
	atexit(cleanup);

	srand(time(NULL));
	time_t start, end;
	//input/output filenames
	char imgfileinname[NAME_MAX] = "";
	char imgfileoutname[NAME_MAX] = "";
	//size_t palsz = 0;
	//number of draw iterations; how many circles/shapes to attempt to draw
	unsigned int iters = 0;
	//color of draw circle
	int color = 0x00000000;
	//coords and radius of circle
	int	drawX, drawY, drawR;
	//upper-left of circle region
	int drawX1, drawY1;
	//width/height of circle region
	int drawW, drawH;
	//number of unsuccessful draws
	int draws = 0;
	/* Resolution at which to check regional color distance (check every xth
	 * pixel). Higher values improve performance while losing quality, but at
	 * too high a value only pixels outside the drawn circle will be checked,
	 * usually causing dist to report no difference and producing weird images.
	 */
	unsigned char distres = DIST_RES_DEFAULT;
	/* drawing randomization mode:
	 * 0 = draw with matching colors at matching coords 
	 * 1 = draw to random coords while sampling colors from original image
	 * 2 = draw purely random colors to random coords (even random alpha!)
	 */
	unsigned char randmode = 1;

	//arg handling
	while ((arg = getopt(argc, argv, "hi:m:o:q:")) != -1)
	{
		switch (arg)
		{
			case 'h':
				printf(HELPTEXT);
				exit(0);
				break;
			case 'i':
				iters = atoi(optarg);
				break;
			case 'm':
				randmode = atoi(optarg);
				if (randmode > 2)
				{
					fprintf(stderr, "Random mode must be 0-2.\n");
					exit(ERR_ARGS);
				}
				break;
			case 'o':
				strncpy(imgfileoutname, optarg, NAME_MAX);
				break;
			case 'q':
				distres = atoi(optarg);
				if (distres < 1 || distres > 10)
				{
					fprintf(stderr, "Image quality must be 1-10.\n");
					exit(ERR_ARGS);
				}
				break;
			case '?':
			default:
				exit(ERR_ARGS);
		}
	}

	//get filename and open file
	if (optind < argc)
	{
		strncpy(imgfileinname, argv[optind], NAME_MAX);
		if (!gdSupportsFileType(imgfileinname, 0))
		{
			fprintf(stderr, "Unsupported input file %s.\n", imgfileinname);
			exit(ERR_FTYPE);
		}
	}
	else
	{
		fprintf(stderr, "Please supply an image filename.\n");
		exit(ERR_ARGS);
	}
	if (imgfileoutname[0] == '\0')
	{
		snprintf(imgfileoutname, NAME_MAX, "out-%d.png", (unsigned)time(NULL));
		printf("Using default filename %s\n", imgfileoutname);
	}

	//get input image and create outputs
	printf("Reading %s...\n", imgfileinname);
	imgin = gdImageCreateFromFile(imgfileinname);
	if (imgin == NULL)
	{
		fprintf(stderr, "Cannot open file %s: ", imgfileinname);
		perror("");
		exit(ERR_FILE_HANDLE);
	}
	imgout1 = gdImageCreateTrueColor(gdImageSX(imgin), gdImageSY(imgin));
	imgout2 = gdImageCreateTrueColor(gdImageSX(imgin), gdImageSY(imgin));
	if (imgout1 == NULL || imgout2 == NULL)
	{
		fprintf(stderr, "Out of memory\n");
		exit(ERR_MEM);
	}

	//palsz = getpalettelist(imgin, pal);
	//pal = realloc(pal, palsz);
	//showpalette(pal, palsz);

	if (iters == 0)
		iters = ITERS_DEFAULT;
	printf("Defaulting to %d iterations.\n", iters);

	printf("Drawing...\n");
	start = clock();
	for (int i = 0; i < iters; i++)
	{
		drawX = rand() % gdImageSX(imgin);
		drawY = rand() % gdImageSY(imgin);
		//get a color at a random coord
		if (randmode < 2)
			color = gdImageGetTrueColorPixel(imgin, drawX, drawY);
		else
			color = rand();

		//paint a circle of that color at another coord (or the same coord)
		if (randmode > 0)
		{
			drawX = rand() % gdImageSX(imgin);
			drawY = rand() % gdImageSY(imgin);
		}
		drawR = rand() % 26 + 5;
		gdImageFilledEllipse(imgout1, drawX, drawY, drawR, drawR, color);

		//compare current and previous to original and keep the closer of the two
		drawX1 = (drawX-drawR);
		drawY1 = (drawY-drawR);
		drawW = (drawR*2);
		drawH = drawR;
		//clip region to image edges
		if (drawX1<0) drawX1 = 0;
		if (drawY1<0) drawY1 = 0;
		if ((drawX1 + drawW) > gdImageSX(imgin)) drawW = (gdImageSX(imgin) - drawX1);
		if ((drawY1 + drawH) > gdImageSX(imgin)) drawH = (gdImageSX(imgin) - drawY1);

		//determine if region "looks closer" to original than before
		if (regiondist(imgout1, imgin,
					drawX1, drawY1, (drawR*2), (drawR*2), distres)
				< regiondist(imgout2, imgin,
					drawX1, drawY1, (drawR*2), (drawR*2), distres))
		{
			//save this draw
			imgCopy(imgout1, imgout2, drawX1, drawY1, (drawR*2), (drawR*2));
			draws++;
		}
		else
		{
			//roll back to previous
			imgCopy(imgout2, imgout1, drawX1, drawY1, (drawR*2), (drawR*2));
		}
	}

	//write final image to file
#ifdef DEBUG
	sprintf(imgfileoutname, "(temp)");
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
	printf("Finished drawing %s in %.3fs.\n%d successful and %d failed draws (%2.2f%% success rate).\n",
			imgfileoutname,
			(double)(end - start) / CLOCKS_PER_SEC,
			draws,
			(iters - draws),
			(double)draws/iters*100);

	/* bad palette reading code here
	//preallocate for palette storage - this is very excessive and could be done dynamically
	pal = calloc((gdImageSX(imgin) * gdImageSY(imgin)), sizeof(int));
	*/

	return 0;
}
