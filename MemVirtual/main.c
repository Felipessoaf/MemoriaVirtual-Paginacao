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

typedef struct frame
{
	int page;
	char M;
	char R;
	clock_t lastUse;
} Frame;

void ReadFile(char *fileName)
{
	unsigned int addr;
	char rw;
	FILE* file;
	file = fopen(fileName, "r");

	if(file == NULL)
	{
		printf("Erro file\n");
	}
	printf("Entrou\n");

	while(fscanf(file, "%x %c", &addr, &rw) == 2)
	{
		//TODO: mapear endereço
		//printf("Endereco: %x\tAcesso: %c\n", addr, rw);
	}

	fclose(file);
}

int main()
{
	int i, status, seg;
	clock_t start, end;
	double cpu_time_used;
	Frame *mainMem;

	seg = shmget (IPC_PRIVATE, 256*sizeof(Frame), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

	mainMem = (Frame*)shmat(seg,0,0);

	start = clock();
	if(fork() != 0)
	{
		if(fork() != 0)
		{
			if(fork() != 0)
			{
				if(fork() == 0)
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

