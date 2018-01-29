/*
 * types.h
 *
 *  Created on: Jan 22, 2018
 *      Author: william
 */

#ifndef TYPES_H_
#define TYPES_H_

#include <stdio.h>

#define OPT_CTX_IMPERIAL (1)
#define OPT_CTX_COMPLETED (2)

struct Point
{
	float x;
	float y;
	unsigned int tool_n;
};

struct ParserContext
{
	unsigned int options;
	float drill_deepness;
	unsigned int drill_feed;
	unsigned int feed;
	float move_height;
	float precision;
	char *src_fileName;
	FILE *src_file;
	char *dst_fileName;
	FILE *dst_file;
	unsigned int toolsCnt;
	float *tools;
	unsigned int pointsCnt;
	struct Point *points;
};

#endif /* TYPES_H_ */
