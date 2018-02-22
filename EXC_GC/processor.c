/*
 * processor.c
 *
 *  Created on: Jan 22, 2018
 *      Author: william
 */

#include "processor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

int InitContext(struct ParserContext* ctx, int argc, char* argv[])
{
	unsigned int i, u_tmp;
	float f_tmp;

	//default values
	ctx->options = 0;
	ctx->drill_deepness = 5;
	ctx->drill_feed = 1200;
	ctx->feed = 1200;
	ctx->move_height = 2;
	ctx->precision = 1000;

	//Reading Context
	for(i=0; i<argc; i++)
	{
		if (strcmp(argv[i], "--help") == 0)
		{
			printf("Usage: EXC_GC <options>\n\nOptions:\n");
			printf("  --help		Show this text\n");
			printf("  -i <filename>		Input excellon file\n");
			printf("  -o <filename>		Output G-Code file\n");
			printf("  -d <value>		Drill deepness\n");
			printf("  -df <value>		Drill feed\n");
			printf("  -f <value>		XY-moving feed\n");
			printf("  -u <value>		precision\n");
			printf("  -h <value>		XY-moving height\n");
			return 1;
		}

		if (strcmp(argv[i], "-i")==0){
			//printf("Input file = %s\n", argv[++i]);
			ctx->src_fileName = argv[++i];
		}
		if (strcmp(argv[i],"-o")==0){
			//printf("Output file = %s\n", argv[++i]);
			ctx->dst_fileName = argv[++i];
		}
		if (strcmp(argv[i],"-d")==0){
			f_tmp = atof(argv[++i]);
			//printf("Override drill deepness = %f\n", f_tmp);
			ctx->drill_deepness = f_tmp;
		}
		if (strcmp(argv[i],"-df")==0){
			u_tmp = atoi(argv[++i]);
			//printf("Override drill feed = %u\n", u_tmp);
			ctx->drill_feed = u_tmp;
		}
		if (strcmp(argv[i],"-f")==0){
			u_tmp = atoi(argv[++i]);
			//printf("Override drill moving feed = %u\n", u_tmp);
			ctx->feed = u_tmp;
		}
		if (strcmp(argv[i],"-u")==0){
			u_tmp = atoi(argv[++i]);
			//printf("Override precision = %u\n", u_tmp);
			ctx->precision = u_tmp;
		}
		if (strcmp(argv[i],"-h")==0){
			f_tmp = atof(argv[++i]);
			if (f_tmp<0)
			{
				fprintf(stderr, "Parameter -h can't be negative!\n");
				return 1;
			}
			ctx->move_height = f_tmp;
		}
	}
	return 0;
}

int Preprocess (struct ParserContext* ctx)
{
	ctx->toolsCnt = 0;
	ctx->pointsCnt = 0;

	char buf[100];
	unsigned int flag = 0;
	//open source file
	ctx->src_file = fopen(ctx->src_fileName, "r");
	if (ctx->src_file == NULL)
	{
		fprintf(stderr, "Error opening source file for preprocessing!\n");
		return 1;
	}

	//count number of tools and points
	while(fgets(buf, 100, ctx->src_file) != NULL){
		if (strcmp(buf, "M48\n") == 0) flag = 1; //встретили начало заголовка
		switch (buf[0]) {
			case '%':
				flag = 0;
				break;
			case 'T':
				if(flag&1) ctx->toolsCnt++;
				break;
			case 'X':
			case 'Y':
				ctx->pointsCnt++;
				break;
			default:
				break;
		}
	}

	fclose(ctx->src_file);

	if (ctx->toolsCnt == 0 || ctx->pointsCnt == 0){
		fprintf(stderr, "No points or/and tools defined!");
		return 1;
	}

	return 0;
}

int Process (struct ParserContext* ctx)
{
	char buf[100];
	unsigned int u_tmp, flag = 0, cTool = 0, cPoint = 0;
	float f1_tmp, f2_tmp;

	//allocate memory for tools and points
	ctx->tools = (float*)malloc(ctx->toolsCnt * sizeof(float));
	ctx->points = (struct Point*)malloc(ctx->pointsCnt * sizeof(struct Point));

	//open source file
	ctx->src_file = fopen(ctx->src_fileName, "r");

	if (ctx->src_file == NULL)
	{
		fprintf(stderr, "Can not open source file for processing!\n");
		return 1;
	}

	while(fgets(buf, 100, ctx->src_file) != NULL && !(ctx->options & OPT_CTX_COMPLETED)){
		switch (buf[0]) {
			case '%':
				flag = 0;
				break;
			case 'M':
				sscanf(buf,"M%u", &u_tmp);
				switch (u_tmp) {
					case 48:
						//header start
						//printf("Start of header detected.\n");
						flag = 1;
						break;
					case 71:
						//printf("Switch to METRIC mode.\n");
						ctx->options &= ~(OPT_CTX_IMPERIAL); //METRIC
						break;
					case 72:
						//printf("Switch to IMPERIAL mode.\n");
						ctx->options |= OPT_CTX_IMPERIAL; //INCHES
						break;
					case 30:
						ctx->options |= OPT_CTX_COMPLETED; //set internal state to "completed"
						//printf("Standard STOP command found.\n");
						break;
					default:
						printf("Unknown M-Command\n");
						break;
				}
				break;
			case 'T':
					if (flag)
					{
						//header area - add tool
						sscanf(buf, "T%uC%f", &u_tmp, &f1_tmp);
						//printf("Adding tool T%u, D = %f\n", u_tmp, f1_tmp);
						ctx->tools[u_tmp-1] = f1_tmp;
					}else{
						//points area - change current numer of tool
						sscanf(buf, "T%u", &u_tmp);
						//printf("Selecting new tool number in toolset (start from 0): %u\n", u_tmp-1);
						cTool = u_tmp-1;
					}
				break;
			case 'X':
				if (!flag)
				{
					//coord found
					sscanf(buf, "X%fY%f", &f1_tmp, &f2_tmp);
					//printf("Adding point for tool %u: %f, %f\n", cTool, f1_tmp, f2_tmp);
					ctx->points[cPoint].tool_n = cTool;
					ctx->points[cPoint].x = f1_tmp/ctx->precision;
					ctx->points[cPoint].y = f2_tmp/ctx->precision;
					cPoint++;
				}
				break;
			default:
				break;
		}
	}

	fclose(ctx->src_file);

	return 0;
}

int GenerateFile(struct ParserContext* ctx)
{
	float x_min, y_min, x_max, y_max;
	unsigned int i, cTool = 65535;
	ctx->dst_file = fopen(ctx->dst_fileName, "w");
	if (ctx->dst_file == NULL)
	{
		fprintf(stderr, "Error opening destination file.\n");
		return 1;
	}

	fprintf(ctx->dst_file, "%s; set measuring mode\n", (ctx->options & OPT_CTX_IMPERIAL) ? "G20" : "G21");
	fprintf(ctx->dst_file, "G90; set absolute positioning\n");
	fprintf(ctx->dst_file, "G92 X0 Y0 Z-%f; reset coordinate to zero\n", ctx->move_height);

	fprintf(ctx->dst_file, "G01 X0 Y0 Z0 F%u\n", ctx->feed);

	//find min/max
	x_min = ctx->points[0].x;
	x_max = x_min;
	y_min = ctx->points[0].y;
	y_max = y_min;
	for (i=1; i<ctx->pointsCnt; i++)
	{
		if (ctx->points[i].x > x_max) x_max =ctx->points[i].x;
		if (ctx->points[i].x < x_min) x_min =ctx->points[i].x;
		if (ctx->points[i].y > y_max) y_max =ctx->points[i].y;
		if (ctx->points[i].y < y_min) y_min =ctx->points[i].y;
	}

	//move across dimensions
	fprintf(ctx->dst_file, "G01 X%f Y%f F%u\n", x_min, y_min, ctx->feed);
	fprintf(ctx->dst_file, "G01 X%f Y%f F%u\n", x_min, y_max, ctx->feed);
	fprintf(ctx->dst_file, "G01 X%f Y%f F%u\n", x_max, y_max, ctx->feed);
	fprintf(ctx->dst_file, "G01 X%f Y%f F%u\n", x_max, y_min, ctx->feed);
	fprintf(ctx->dst_file, "G01 X%f Y%f F%u\n", x_min, y_min, ctx->feed);
	fprintf(ctx->dst_file, "G01 X0 Y0 F%u\n", ctx->feed);

	for (i=0; i < ctx->pointsCnt; i++)
	{
		//move XY
		fprintf(ctx->dst_file, "G01 X%f Y%f F%u\n", ctx->points[i].x, ctx->points[i].y, ctx->feed);
		//do it when tool is new
		if (ctx->points[i].tool_n != cTool)
		{
			cTool = ctx->points[i].tool_n;
			//move tool up
			fprintf(ctx->dst_file, "G01 Z50 F%u\n", ctx->feed);
			//wait
			fprintf(ctx->dst_file, "M0 Ch. T%u, D%f\n", cTool+1, ctx->tools[cTool]);
			//move tool down to -mive_height
			fprintf(ctx->dst_file, "G01 Z-%f F%u\n", ctx->move_height, ctx->feed);
			//wait for fix tool
			fprintf(ctx->dst_file, "M0 Fix tool!\n");
			//move to zero Z position
			fprintf(ctx->dst_file, "G01 Z0 F%u\n", ctx->feed);
			//wait for spindle on
			fprintf(ctx->dst_file, "M0 Turn on spindle!\n");
		}

		fprintf(ctx->dst_file, "G01 Z%f F%u\n", -ctx->drill_deepness, ctx->drill_feed);
		fprintf(ctx->dst_file, "G01 Z0 F%u\n", ctx->feed);
	}
	fprintf(ctx->dst_file, "M107 ; fan off\nG01 Z50 F%u\nG01 X0 Y0 ; home XY\n", ctx->feed);

	fclose(ctx->dst_file);

	return 0;
}
