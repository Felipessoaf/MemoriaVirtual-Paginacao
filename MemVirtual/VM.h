/*
 * VM.h
 *
 *  Created on: 8 de dez de 2017
 *      Author: felipessoaf
 */

#ifndef VM_H_
#define VM_H_

unsigned int trans(int pid, unsigned int page, unsigned int offset, char rw);

#endif /* VM_H_ */
