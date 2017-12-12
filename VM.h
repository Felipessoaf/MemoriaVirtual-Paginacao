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
#include <sys/time.h>
#include <limits.h>
#include <sys/sem.h>

#define MAXPAGE 65536
#define RESETTIMER 10
#define MAXFRAME 256
//#define LOG

typedef struct page
{
	int frame;
	char inMemory;
} Page;

typedef struct frame
{
	int page;
	char M;
	struct timeval lastUse;
	int pid;
} Frame;

void trans(int pid, unsigned int page, unsigned int offset, char rw);

void Init();

void End();

#endif /* VM_H_ */
