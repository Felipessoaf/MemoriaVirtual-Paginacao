/*
 * VM.c
 *
 *  Created on: 8 de dez de 2017
 *      Author: felipessoaf
 */
#include "VM.h"

static pthread_mutex_t lock;

static unsigned concatenate(unsigned x, unsigned y);

void trans(int pid, unsigned int page, unsigned int offset, char rw)
{
	int i, seg, segPage, segFrame, *currentPage;
	Page *pageTable;
	Frame *mainMem;

	seg = shmget (pid, MAXPAGE*sizeof(Page), 0666);
	if(seg < 0)
	{
		seg = shmget (pid, MAXPAGE*sizeof(Page), IPC_CREAT | 0666);
		if(seg < 0)
		{
			printf("Erro seg");
			exit(1);
		}
		pageTable = (Page*)shmat(seg,0,0);
		for(i = 0; i < MAXPAGE; i++)
		{
			pageTable[i].frame = -1;
			pageTable[i].inMemory = 0;
		}
	}
	else
	{
		pageTable = (Page*)shmat(seg,0,0);
	}

	segPage = shmget (4321, sizeof(int), 0666);
	if(segPage < 0)
	{
		printf("Erro segPage");
		exit(1);
	}
	currentPage = (int*)shmat(segPage,0,0);

	segFrame = shmget (1234, 256*sizeof(Frame), 0666);
	if(segFrame < 0)
	{
		printf("Erro segFrame");
		exit(1);
	}
	mainMem = (Frame*)shmat(segFrame,0,0);

	printf("ANTES REGIAO CRITICA\n");
	//REGIAO CRITICA--------------------------------------
    pthread_mutex_lock(&lock);
	*currentPage = page;

	//procura na tabela se a page está associada já a um frame
	if(pageTable[page].inMemory == 0)
	{
		//se nao tiver, manda SIGUSR1 pro GM pra avisar que deu page fault
		kill(getppid(), SIGUSR1);
		sleep(1);
		printf("DPS DO PAGE FAULT\n");
	}
	else
	{
		//marcar no frame usado o tempo
		mainMem[pageTable[page].frame].lastUse = clock();
	}
	//marcar no frame usado o RW
	if(rw == 'W')
	{
		mainMem[pageTable[page].frame].M = 1;
	}

	printf("Processo: %d\tEndereco Fisico: %x\tAcesso: %c\n", pid, concatenate(pageTable[page].frame, offset), rw);

	//REGIAO CRITICA--------------------------------------
    pthread_mutex_unlock(&lock);
	printf("DEPOIS REGIAO CRITICA\n");

	shmdt (mainMem);
	shmdt (currentPage);
	shmdt (pageTable);
}

unsigned concatenate(unsigned x, unsigned y) {
    unsigned pow = 10;
    while(y >= pow)
        pow *= 10;
    return x * pow + y;
}

void Init()
{
	pthread_mutexattr_t attrmutex;

	pthread_mutexattr_init(&attrmutex);
	pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&lock, &attrmutex);

    pthread_mutex_destroy(&lock);
    pthread_mutexattr_destroy(&attrmutex);
}
