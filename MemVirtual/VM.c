/*
 * VM.c
 *
 *  Created on: 8 de dez de 2017
 *      Author: felipessoaf
 */

static unsigned concatenate(unsigned x, unsigned y);

void trans(int pid, unsigned int page, unsigned int offset, char rw)
{
	//procura na tabela se a page está associada já a um frame
	//se tiver, imprime o endereço convertido direto
	//senao, manda SIGUSR1 pro GM pra avisar que deu page fault

	printf("Processo: %d\tEndereco Fisico: %x\tAcesso: %c\n", pid, concatenate(frame, offset), rw);
}

unsigned concatenate(unsigned x, unsigned y) {
    unsigned pow = 10;
    while(y >= pow)
        pow *= 10;
    return x * pow + y;
}
