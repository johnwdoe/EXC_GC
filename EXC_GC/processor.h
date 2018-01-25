/*
 * processor.h
 *
 *  Created on: Jan 22, 2018
 *      Author: william
 */

#ifndef PROCESSOR_H_
#define PROCESSOR_H_

#include "types.h"

int InitContext(struct ParserContext* ctx, int argc, char* argv[]);

int Preprocess (struct ParserContext* ctx);

int Process (struct ParserContext* ctx);

int GenerateFile(struct ParserContext* ctx);

#endif /* PROCESSOR_H_ */
