#ifndef __STDLIB_H
#define __STDLIB_H

#define	RAND_MAX	0x7fff

void *malloc(unsigned int nbytes);
void free(void *ap);
const int rand(void);
void srand(unsigned int seed);

#endif
