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
	printf("Drill deepness: %f\n", ctx.drill_deepness);
	printf("Drill FeedForward/FeedBack: %u/%u\n", ctx.drill_feed, ctx.drill_feed_back);
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
	//preprocessing
	while(fgets(buf, 100, ctx.src_file) != NULL)
	{
		if (strcmp(buf, "%\n") == 0)
		{
			if (flag & F_HDR_READING)
			{
				flag |= F_HDR_READY;
				flag &= (~F_HDR_READING);
				printf("End of header detected.\n");
				fprintf(ctx.dst_file, "G90 ; Setting absolute positioning\nG92 X0 Y0 Z0 ; Reset all axis to zero\n");
			}
		}
		else if (strcmp(buf, "M48\n") == 0)
		{
			flag |= F_HDR_READING;
			printf("Start of header detected.\n");
		}
		else if (strcmp(buf, "M71\n") == 0)
		{
			fprintf(ctx.dst_file, "G21 ; Set metric mode\n");
		}
		else if (strcmp(buf, "M72\n") == 0)
		{
			fprintf(ctx.dst_file, "G20 ; Set imperial mode\n");
		}
		else if (buf[0] == 'T')
		{
			if (flag & F_HDR_READING)
			{
				//add tool
				sscanf(buf, "T%uC%f", &u_tmp, &f_tmp);
				tools[u_tmp-1] = f_tmp;
				printf(";Tool N %u with diameter %f added.\n", u_tmp, f_tmp);
			}
			else if (flag & F_HDR_READY)
			{
				//select tool
				sscanf(buf, "T%u", &u_tmp);
				fprintf(ctx.dst_file, "M117 T%u, D%f\n", u_tmp, tools[u_tmp-1]);
			}
		}
		else if (buf[0] == 'X')
		{
			//reading drill coord
			sscanf(buf, "X%fY%f", &f_tmp, &f_tmp2);
			//fix value
			f_tmp /= ctx.precision;
			f_tmp2 /= ctx.precision;
			//printf("Drill at %f, %f\n", f_tmp, f_tmp2);
			fprintf(ctx.dst_file, "G01 X%f Y%f F%u\n", f_tmp, f_tmp2, ctx.drill_feed);
			fprintf(ctx.dst_file, "G01 Z%f F%u\n", -ctx.drill_deepness, ctx.drill_feed);
			fprintf(ctx.dst_file, "G01 Z0 F%u\n", ctx.drill_feed);
		}
		else if (strcmp(buf, "M30\n") == 0)
		{
			printf("Adding final GCode.\n");
			fprintf(ctx.dst_file, "M107 ; fan off\nG01 Z100 F%u\nG28 X Y ; home XY\n", ctx.drill_feed);
		}
		else
		{
			printf("What can I do with this string? :%s", buf);
		}

	}
	fclose(ctx.src_file);
	fclose(ctx.dst_file);

	return 0;
}

