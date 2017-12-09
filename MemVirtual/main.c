/*
 * main.c
 *
 *  Created on: 7 de dez de 2017
 *      Author: felipessoaf
 */

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

#include "VM.h"

#define MAXFRAME 256
#define RESETTIMER 30

int LRU(Frame *mainMem)
{
	int i, removeIndex;//seg,
	clock_t minTime;
//	Frame *mainMem;

	minTime = clock();
	removeIndex = -1;

//	seg = shmget (1234, 256*sizeof(Frame), 0666);
//	if(seg < 0)
//	{
//		printf("Error shmget\n");
//		exit(1);
//	}
//
//	mainMem = (Frame*)shmat(seg,0,0);

	for(i = 0; i < MAXFRAME; i++)
	{
		if(mainMem[i].lastUse < minTime)
		{
			removeIndex = i;
			minTime = mainMem[i].lastUse;
		}
	}

	if(removeIndex == -1)
	{
		printf("Error LRU\n");
		exit(1);
	}

	return minTime;
}

void PageFault(int sig, siginfo_t* info, void* vp)
{
	int segPage, segPageTable, segFrame, *currentPage, segIndex, *lastIndex, pidLost, removeIndex;
	Page *pageTable;
	Frame *mainMem;

	segIndex = shmget (8462, sizeof(int), 0666);
	if(segIndex < 0)
	{
		printf("Erro segIndex");
		exit(1);
	}
	lastIndex = (int*)shmat(segIndex,0,0);

	segPage = shmget (4321, sizeof(int), 0666);
	if(segPage < 0)
	{
		printf("Erro segPage");
		exit(1);
	}
	currentPage = (int*)shmat(segPage,0,0);

	segPageTable = shmget (info->si_pid, MAXPAGE*sizeof(Page), 0666);
	if(segPageTable < 0)
	{
		printf("Erro segPageTable");
		exit(1);
	}
	pageTable = (Page*)shmat(segPageTable,0,0);

	segFrame = shmget (1234, 256*sizeof(Frame), 0666);
	if(segFrame < 0)
	{
		printf("Erro segFrame");
		exit(1);
	}
	mainMem = (Frame*)shmat(segFrame,0,0);

	printf("PAGE FAULT\n");

	//manda SIGSTOP pro sender
	kill(info->si_pid, SIGSTOP);

	//ve se tem espaço na memoria
	if(*lastIndex < MAXFRAME)
	{
		//atualiza tabela
		pageTable[*currentPage].frame = *lastIndex;
		pageTable[*currentPage].inMemory = 1;
		//se tiver, marca o lugar como usado
		mainMem[*lastIndex].page = *currentPage;
		mainMem[*lastIndex].pid = info->si_pid;
		mainMem[*lastIndex].lastUse = clock();
		(*lastIndex)++;
		sleep(1);
	}
	else
	{
		//senao, chama o LRU pra decidir qual vai sair
		removeIndex = LRU(mainMem);
		sleep(1);
		pidLost = mainMem[removeIndex].pid;
		//se tiver modificado, marca pra esperar mais 1 segundo
		if(mainMem[removeIndex].M == 1)
		{
			sleep(1);
		}

		//atualiza tabela
		pageTable[*currentPage].frame = removeIndex;
		pageTable[*currentPage].inMemory = 1;

		//update page a ser retirada
		*currentPage = mainMem[removeIndex].page;
		//tira ele, e bota o novo no lugar
		mainMem[removeIndex].page = *currentPage;
		mainMem[removeIndex].pid = info->si_pid;
		mainMem[removeIndex].lastUse = clock();

		//manda SIGUSR2 pro processo que perdeu o frame
		kill(pidLost, SIGUSR2);
	}

	printf("\n\n\nLIBERANDO PAGE FAULT\n\n\n");
	//manda SIGCONT pro sender
	kill(info->si_pid, SIGCONT);

	shmdt (mainMem);
	shmdt (currentPage);
	shmdt (lastIndex);
	shmdt (pageTable);
}

void LostPage()
{
	//atualiza na tabela de paginas
	int seg, segPage, *currentPage;
	Page *pageTable;

	seg = shmget (getpid(), MAXPAGE*sizeof(Page), 0666);
	if(seg < 0)
	{
		printf("Erro seg");
		exit(1);
	}
	pageTable = (Page*)shmat(seg,0,0);

	segPage = shmget (4321, sizeof(int), 0666);
	if(segPage < 0)
	{
		printf("Erro segPage");
		exit(1);
	}
	currentPage = (int*)shmat(segPage,0,0);

	pageTable[*currentPage].frame = -1;
	pageTable[*currentPage].inMemory = 0;

	shmdt (pageTable);
	shmdt (currentPage);
}

void ReadFile(char *fileName)
{
	int pid;
	unsigned int addr, page, offset;
	char rw;
	FILE* file;

	signal(SIGUSR2, LostPage);

	file = fopen(fileName, "r");
	pid = getpid();

	if(file == NULL)
	{
		printf("Erro file\n");
	}
//	printf("Entrou\n");

	while(fscanf(file, "%x %c", &addr, &rw) == 2)
	{
		page = addr>>16;
		offset = ((addr<<16)>>16);

		trans(pid, page, offset, rw);
	}

	fclose(file);
}

int main()
{
	int i, status, seg, segPage, segIndex, *currentPage, *lastIndex;
	clock_t start, end;
	double cpu_time_used;
	Frame *mainMem;
	struct sigaction sa;

	seg = shmget (1234, MAXFRAME*sizeof(Frame), IPC_CREAT | 0666);
	segPage = shmget (4321, sizeof(int), IPC_CREAT | 0666);
	segIndex = shmget (8462, sizeof(int), IPC_CREAT | 0666);

	if(seg < 0)
	{
		printf("Erro seg");
		exit(1);
	}
	if(segPage < 0)
	{
		printf("Erro segPage");
		exit(1);
	}
	if(segIndex < 0)
	{
		printf("Erro segIndex");
		exit(1);
	}

	mainMem = (Frame*)shmat(seg,0,0);
	currentPage = (int*)shmat(segPage,0,0);
	lastIndex = (int*)shmat(segIndex,0,0);
	*lastIndex = 0;

	printf("Start clock\n");
	start = clock();
	if(fork() != 0)
	{
		if(fork() != 0)
		{
			if(fork() != 0)
			{
				if(fork() != 0)
				{
					//signal(SIGUSR2, PageFault);
					Init();
					sa.sa_handler = &PageFault;
					sigemptyset(&sa.sa_mask);
					sa.sa_flags = SA_RESTART | SA_SIGINFO;
					if (sigaction(SIGUSR2, &sa, 0) == -1) {
					  perror(0);
					  exit(1);
					}
				}
				else
				{
					//filho4
					ReadFile("simulador.log");
					exit(0);
				}
			}
			else
			{
				//filho3
				ReadFile("compressor.log");
				exit(0);
			}
		}
		else
		{
			//filho2
			ReadFile("matriz.log");
			exit(0);
		}
	}
	else
	{
		//filho1
		ReadFile("compilador.log");
		exit(0);
	}


	for(i = 0; i < 4; i++)
	{
		waitpid(-1, &status, 0);
	}

	end = clock();
	cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

	printf("Duracao em segundos: %f\n", cpu_time_used);



	shmdt (mainMem);
	shmdt (currentPage);
	shmdt (lastIndex);

	shmctl (seg, IPC_RMID, 0);
	shmctl (segPage, IPC_RMID, 0);
	shmctl (segIndex, IPC_RMID, 0);

	return 0;
}

