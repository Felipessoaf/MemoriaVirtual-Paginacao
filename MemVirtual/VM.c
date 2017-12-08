/*
 * VM.c
 *
 *  Created on: 8 de dez de 2017
 *      Author: felipessoaf
 */
#include "VM.h"

static unsigned concatenate(unsigned x, unsigned y);

void trans(int pid, unsigned int page, unsigned int offset, char rw)
{
	int i, seg, segPage, *currentPage;
	Page *pageTable;

//	seg = shmget (pid, MAXPAGE*sizeof(Page), 0666);
//	if(seg < 0)
//	{
		seg = shmget (pid, MAXPAGE*sizeof(Page), IPC_CREAT | 0666);
//	}
//	else
//	{
//		pageTable = {[0 ... MAXPAGE-1].frame = -1,[0 ... MAXPAGE-1].inMemory = 0};
//	}

	pageTable = (Page*)shmat(seg,0,0);

	segPage = shmget (4321, sizeof(int), 0666);
	currentPage = (int*)shmat(segPage,0,0);
	*currentPage = page;

	//procura na tabela se a page está associada já a um frame
	if(pageTable[page].inMemory < 0)
	{
		//se nao tiver, manda SIGUSR1 pro GM pra avisar que deu page fault
		kill(getppid(), SIGUSR1);
		sleep(1);
	}

	printf("Processo: %d\tEndereco Fisico: %x\tAcesso: %c\n", pid, concatenate(pageTable[page].frame, offset), rw);
}

unsigned concatenate(unsigned x, unsigned y) {
    unsigned pow = 10;
    while(y >= pow)
        pow *= 10;
    return x * pow + y;
}
