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

#define F_HDR_READING	(1)
#define F_HDR_READY		(2)

struct ParserContext
{
	float drill_deepness;
	unsigned int drill_feed;
	//unsigned int drill_feed_back;
	float precision;
	FILE *src_file;
	FILE *dst_file;

} ctx;

struct Point
{
	float x;
	float y;
	unsigned int tool_n;
};

uint8_t flag = 0;
char buf[100];
unsigned int u_tmp;
float tools[100], f_tmp, f_tmp2;


int main(int argc, char* argv[])
{
	//default values
	ctx.drill_deepness = 5;
	ctx.drill_feed = 1200;
	//ctx.drill_feed_back = 0;
	ctx.precision = 1000;
	int i;

	printf("Parse input parameters...\n");
	for(i=0; i<argc; i++)
	{
		if (strcmp(argv[i], "-i")==0){
			printf("Input file = %s\n", argv[++i]);
			ctx.src_file=fopen(argv[i], "r");
		}
		if (strcmp(argv[i],"-o")==0){
			printf("Output file = %s\n", argv[++i]);
			ctx.dst_file=fopen(argv[i], "w");
		}
		if (strcmp(argv[i],"-d")==0){
			f_tmp = atof(argv[++i]);
			printf("Override drill deepness = %f\n", f_tmp);
			ctx.drill_deepness = f_tmp;
		}
		if (strcmp(argv[i],"-f")==0){
			u_tmp = atoi(argv[++i]);
			printf("Override drill feed = %u\n", u_tmp);
			ctx.drill_feed = u_tmp;
		}
		if (strcmp(argv[i],"-u")==0){
			u_tmp = atoi(argv[++i]);
			printf("Override precision = %u\n", u_tmp);
			ctx.precision = u_tmp;
		}
	}
	if (ctx.src_file == NULL)
	{
		printf("Error opening input file.\n");
		return 1;
	}

	if (ctx.dst_file == NULL)
	{
		printf("Error opening output file.\n");
		return 1;
	}

	//reading & parsing
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

