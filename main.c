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

int pageFaultCount;

int LRU(Frame *mainMem)
{
	int i, removeIndex;//seg,
	struct timeval minTime;
	double elapsedTime;

	gettimeofday(&minTime, NULL);
	removeIndex = -1;

	for(i = 0; i < MAXFRAME; i++)
	{
		elapsedTime = (minTime.tv_sec - mainMem[i].lastUse.tv_sec) * 1000.0;      // sec to ms
		elapsedTime += (minTime.tv_usec - mainMem[i].lastUse.tv_usec) / 1000.0;   // us to ms
		if(elapsedTime > 0)
		{
			removeIndex = i;
			minTime.tv_sec = mainMem[i].lastUse.tv_sec;
			minTime.tv_usec = mainMem[i].lastUse.tv_usec;
		}
	}

	if(removeIndex == -1)
	{
		printf("Error LRU\n");
		exit(1);
	}

	return removeIndex;
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

#ifdef LOG
	printf("%d: PAGE FAULT\n", info->si_pid);
	printf("%d: PARANDO SENDER\n",info->si_pid);
#endif
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
		mainMem[*lastIndex].M = 0;
		gettimeofday(&mainMem[*lastIndex].lastUse, NULL);
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

		//tira ele, e bota o novo no lugar
		mainMem[removeIndex].page = *currentPage;
		mainMem[removeIndex].pid = info->si_pid;
		mainMem[removeIndex].M = 0;
		gettimeofday(&mainMem[removeIndex].lastUse, NULL);

		//update page a ser retirada
		*currentPage = mainMem[removeIndex].page;

		//manda SIGUSR2 pro processo que perdeu o frame
		kill(pidLost, SIGUSR2);
	}

#ifdef LOG
	printf("%d: LIBERANDO PAGE FAULT\n", info->si_pid);
#endif
	//manda SIGCONT pro sender
	kill(info->si_pid, SIGCONT);
#ifdef LOG
	printf("%d: SENDER LIBERADO\n",info->si_pid);
#endif
	kill(info->si_pid, SIGCONT);

	pageFaultCount++;

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
	int pid, counter;
	unsigned int addr, page, offset;
	char rw;
	FILE* file;

	signal(SIGUSR2, LostPage);

	file = fopen(fileName, "r");
	pid = getpid();
	counter = 0;
	printf("%d: %s\n",pid,fileName);

	if(file == NULL)
	{
		printf("Erro file\n");
	}

	while(fscanf(file, "%x %c", &addr, &rw) == 2)
	{
		page = addr>>16;
		offset = ((addr<<16)>>16);

		printf("%d: Trans %d\n",pid,counter);
		trans(pid, page, offset, rw);
		counter++;
	}

	fclose(file);
}

int main()
{
	int i, status, seg, segPage, segIndex, segCounter, *currentPage, *lastIndex, *counter;
	Frame *mainMem;
	struct sigaction sa, old_action;
	struct timeval t1, t2;
    double elapsedTime;

	seg = shmget (1234, MAXFRAME*sizeof(Frame), IPC_CREAT | 0666);
	segPage = shmget (4321, sizeof(int), IPC_CREAT | 0666);
	segIndex = shmget (8462, sizeof(int), IPC_CREAT | 0666);
	segCounter = shmget (2468, sizeof(int), IPC_CREAT | 0666);

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
	if(segCounter < 0)
	{
		printf("Erro segCounter");
		exit(1);
	}

	mainMem = (Frame*)shmat(seg,0,0);
	currentPage = (int*)shmat(segPage,0,0);
	lastIndex = (int*)shmat(segIndex,0,0);
	counter = (int*)shmat(segCounter,0,0);
	*lastIndex = 0;
	*counter = 0;

	printf("Start clock\n");
	gettimeofday(&t1, NULL);

	if(fork() != 0)
	{
		if(fork() != 0)
		{
			if(fork() != 0)
			{
				if(fork() != 0)
				{
					Init();
					pageFaultCount = 0;
					sa.sa_sigaction = &PageFault;
					sigemptyset(&sa.sa_mask);
					sa.sa_flags = SA_RESTART | SA_SIGINFO;
					if (sigaction(SIGUSR1, &sa, &old_action) == -1) {
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

	gettimeofday(&t2, NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
	printf("Duracao em segundos: %f\n", elapsedTime/1000);
	printf("Page Faults: %d\n", pageFaultCount);

	End();

	shmdt (mainMem);
	shmdt (currentPage);
	shmdt (lastIndex);
	shmdt (counter);

	shmctl (seg, IPC_RMID, 0);
	shmctl (segPage, IPC_RMID, 0);
	shmctl (segIndex, IPC_RMID, 0);
	shmctl (segCounter, IPC_RMID, 0);

	return 0;
}

