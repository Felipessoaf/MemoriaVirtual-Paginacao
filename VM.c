/*
 * VM.c
 *
 *  Created on: 8 de dez de 2017
 *      Author: felipessoaf
 */
#include "VM.h"

union semun
{
	int val;
	struct semid_ds *buf;
	ushort *array;
};

// inicializa o valor do semáforo
int setSemValue(int semId);
// remove o semáforo
void delSemValue(int semId);
// operação P
int semaforoP(int semId);
//operação V
int semaforoV(int semId);

static unsigned concatenate(unsigned x, unsigned y);

void trans(int pid, unsigned int page, unsigned int offset, char rw)
{
	int i, seg, segPage, segFrame, segCounter, *currentPage, *counter;
	int semId;
	Page *pageTable;
	Frame *mainMem;
	struct timeval t1;

	semId = semget (8752, 1, 0666);
	if(semId < 0)
	{
		printf("Erro semId");
		exit(1);
	}

	segCounter = shmget (2468, sizeof(int), 0666);
	if(segCounter < 0)
	{
		printf("Erro segCounter");
		exit(1);
	}
	counter = (int*)shmat(segCounter,0,0);

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

#ifdef LOG
	printf("%d: ANTES REGIAO CRITICA\n", pid);
#endif
	//REGIAO CRITICA--------------------------------------
//    pthread_mutex_lock(&lock);
	semaforoP(semId);

	if((*counter) >= RESETTIMER)
	{
		gettimeofday(&t1, NULL);
		for(i = 0; i < MAXFRAME; i++)
		{
			mainMem[i].lastUse.tv_sec = t1.tv_sec;
			mainMem[i].lastUse.tv_usec = t1.tv_usec;
		}
	}

	*currentPage = page;

	//procura na tabela se a page está associada já a um frame
	if(pageTable[page].inMemory == 0)
	{
		//se nao tiver, manda SIGUSR1 pro GM pra avisar que deu page fault
		kill(getppid(), SIGUSR1);
//		sleep(1);
#ifdef LOG
		printf("%d: VOU ME PARAR\n",pid);
#endif
		kill(pid, SIGSTOP);
#ifdef LOG
		printf("%d: VOLTEI\n",pid);
		printf("%d: DPS DO PAGE FAULT\n",pid);
#endif
	}
	else
	{
		//marcar no frame usado o tempo
		gettimeofday(&mainMem[pageTable[page].frame].lastUse, NULL);
#ifdef LOG
		printf("%d: JA EM MEMORIA\n",pid);
#endif
	}
	//marcar no frame usado o RW
	if(rw == 'W')
	{
		mainMem[pageTable[page].frame].M = 1;
	}

	printf("Processo: %d\tEndereco Fisico: %x\tAcesso: %c\n", pid, concatenate(pageTable[page].frame, offset), rw);
	(*counter)++;

	//REGIAO CRITICA--------------------------------------
//    pthread_mutex_unlock(&lock);
	semaforoV(semId);
#ifdef LOG
	printf("%d: DEPOIS REGIAO CRITICA\n", pid);
#endif

	shmdt (mainMem);
	shmdt (currentPage);
	shmdt (pageTable);
	shmdt (counter);
}

unsigned concatenate(unsigned x, unsigned y) {
    unsigned pow = 10;
    while(y >= pow)
        pow *= 10;
    return x * pow + y;
}

void Init()
{
//	pthread_mutexattr_t attrmutex;
//
//	pthread_mutexattr_init(&attrmutex);
//	pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);
//
//    pthread_mutex_init(&lock, &attrmutex);
//
//    pthread_mutex_destroy(&lock);
//    pthread_mutexattr_destroy(&attrmutex);
	int semId;

	semId = semget (8752, 1, 0666 | IPC_CREAT);
	setSemValue(semId);
}

void End()
{
	int semId;
	semId = semget (8752, 1, 0666);
	delSemValue(semId);
}

int setSemValue(int semId)
{
	union semun semUnion;
	semUnion.val = 1;
	return semctl(semId, 0, SETVAL, semUnion);
}

void delSemValue(int semId)
{
	union semun semUnion;
	semctl(semId, 0, IPC_RMID, semUnion);
}

int semaforoP(int semId)
{
	struct sembuf semB;
	semB.sem_num = 0;
	semB.sem_op = -1;
	semB.sem_flg = SEM_UNDO;
	semop(semId, &semB, 1);
	return 0;
}

int semaforoV(int semId)
{
	struct sembuf semB;
	semB.sem_num = 0;
	semB.sem_op = 1;
	semB.sem_flg = SEM_UNDO;
	semop(semId, &semB, 1);
	return 0;
}
