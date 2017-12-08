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

#include "VM.h"

typedef struct frame
{
	int page;
	char M;
	char R;
	clock_t lastUse;
} Frame;

typedef struct page
{
	int frame;
	char inMemory;
} Page;

void PageFault()
{
	//REGIAO CRITICA--------------------------------------
	//ve se tem espaço na memoria
	//se tiver, marca o lugar como usado
	//senao, chama o LRU pra decidir qual vai sair,
	//se tiver modificado, marca pra esperar mais 1 segundo
	//espera 1 ou 2 segundos, dependendo do criterio acima
	//tira ele, e bota o novo no lugar
	//manda SIGUSR2 pro processo que perdeu o frame
	//REGIAO CRITICA--------------------------------------
}

void LostPage()
{
	//atualiza na tabela de paginas
}

void ReadFile(char *fileName)
{
	int pid;
	unsigned int addr, page, offset;
	char rw;
	FILE* file;

	signal(SIGUSR2, LostPage);

	file = fopen(fileName, "r");

	if(file == NULL)
	{
		printf("Erro file\n");
	}
	printf("Entrou\n");

	while(fscanf(file, "%x %c", &addr, &rw) == 2)
	{
		//TODO: mapear endereço
		page = addr>>16;
		offset = ((addr<<16)>>16);
		pid = getpid();

		trans(pid, page, offset, rw);
	}

	fclose(file);
}

int main()
{
	int i, status, seg;
	clock_t start, end;
	double cpu_time_used;
	Frame *mainMem;

	seg = shmget (1234, 256*sizeof(Frame), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

	mainMem = (Frame*)shmat(seg,0,0);

	start = clock();
	if(fork() != 0)
	{
		if(fork() != 0)
		{
			if(fork() != 0)
			{
				if(fork() != 0)
				{
					signal(SIGUSR2, PageFault);
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
	return 0;
}

