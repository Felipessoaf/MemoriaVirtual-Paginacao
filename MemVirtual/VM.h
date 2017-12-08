/*
 * VM.h
 *
 *  Created on: 8 de dez de 2017
 *      Author: felipessoaf
 */

#ifndef VM_H_
#define VM_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <time.h>
#include <limits.h>

#define MAXPAGE 65536

typedef struct page
{
	int frame;
	char inMemory;
} Page = {-1,0};

unsigned int trans(int pid, unsigned int page, unsigned int offset, char rw);

#endif /* VM_H_ */
