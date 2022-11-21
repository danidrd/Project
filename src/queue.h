
#include <pthread.h>
#ifndef __BOOL__
#define __BOOL__
typedef enum { FALSE, TRUE } bool;
#endif

/*******************************************************************************
	Struttura coda:
	q      --> puntatore all'array circolare contenente gli elementi
	first  --> puntatore al primo elemento della coda
	last   --> puntatore all'ultimo elemento della coda
	dim    --> dimensione massima della coda
	num_el --> numero di elementi presenti nella coda
    mtx    --> mutex della coda
    empty  --> variabile di condizione empty
    full   --> variabile di condizione full
*******************************************************************************/
typedef struct queue{
	char **q;
	char **first;
	char **last;
	char **clear;
	int num_clear;
	int dim;
	int num_el;
	int stop;
    pthread_mutex_t mtx;
    pthread_cond_t empty;
    pthread_cond_t full;
} queue_t;


queue_t* new_queue(int dim);

void delete_queue(queue_t *coda);

int insert(queue_t *coda, char *elem);

char *extract(queue_t *coda);

int empty(queue_t *coda);

int full(queue_t *coda);