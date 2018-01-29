/*
 * main.c
 *
 *  Created on: Jan 2, 2018
 *      Author: william
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "types.h"
#include "processor.h"

#define F_HDR_READING	(1)
#define F_HDR_READY		(2)


struct ParserContext ctx;

unsigned int flag = 0;
char buf[100];
unsigned int u_tmp;
float tools[100], f_tmp, f_tmp2;


int main(int argc, char* argv[])
{
	printf("Initializing context...\n");
	if (InitContext(&ctx, argc, argv) != 0) return 1;

	printf("Context data:\nSouce file: %s\nDestination file: %s\n", ctx.src_fileName, ctx.dst_fileName);
	printf("Moving height: %f\n", ctx.move_height);
	printf("Drill deepness: %f\n", ctx.drill_deepness);
	printf("Move/Drill Feed: %u/%u\n", ctx.feed, ctx.drill_feed);
	printf("Units precision: %f\n\n", ctx.precision);

	printf("Start pre-processing...\n");
	if (Preprocess(&ctx) != 0) return 1;
	printf("Number of tools found: %u\nNumber of points found: %u\n\n", ctx.toolsCnt, ctx.pointsCnt);

	printf("Start processing...\n");
	if (Process(&ctx) != 0) return 1;
	if (ctx.options & OPT_CTX_COMPLETED)
	{
		printf("Standard processing completion.\n");
	} else{
		printf("WARNING! Processing stopped at EOF!\n");
	}
	printf("Measuring mode: %s\n\n", (ctx.options&OPT_CTX_IMPERIAL) ? "IMPERIAL" : "METRIC");

	printf("Start generating output file...\n");
	if (GenerateFile(&ctx) != 0) return 1;

	return 0;
}

