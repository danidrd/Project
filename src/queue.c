/*
	In questa libreria ho messo le funzioni per la gestione di code di stringhe
	implementate con array circolari di dimensione fissa.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include "queue.h"



/*******************************************************************************
	Macro per settare i valori nella struttura queue_t
*******************************************************************************/
#define RE_SET( CODA, NEW_Q, NEW_DIM, FIRST, LAST, NUM_EL, STOP)		\
	do{															\
		(CODA)->q = (NEW_Q);									\
		(CODA)->first = (FIRST);								\
		(CODA)->last = (LAST);									\
		(CODA)->dim = (NEW_DIM);								\
		(CODA)->num_el = (NUM_EL);								\
		(CODA)->stop = (STOP);									\
	} while(0)																


/*******************************************************************************
	Funzione che prende un intero (dim) e crea una nuova coda
	con dim posti disponibili
*******************************************************************************/
queue_t*
new_queue(int dim)
{
	errno = 0;
	queue_t *new; char **que;
	if( !(new=malloc(sizeof(queue_t))) || !(que=calloc(dim,sizeof(char*))) ){
		if( new ) free(new);
		errno = ENOMEM;
		perror("malloc");
		return NULL;
	}
	if(!(new->clear = malloc(sizeof(char*)))){
		errno = ENOMEM;
		perror("malloc");
		return NULL;
	}
	new->num_clear = 1;
    pthread_mutex_init(&(new->mtx), NULL);
    pthread_cond_init(&(new->empty), NULL);
    pthread_cond_init(&(new->full), NULL);

	RE_SET( new, que, dim, que, que, 0, 0);
	return new;
}


/*******************************************************************************
	Funzione che prende una coda e libera la memoria della coda.
*******************************************************************************/

void
delete_queue(queue_t *coda){
    if(coda){  
        /*for (size_t i = 0; i < coda->dim; i++)
        {
            if((coda->q)[i])free((coda->q)[i]);
        }*/
		coda->clear = realloc(coda->clear, sizeof(char*) * coda->num_clear-1);
		for(size_t i = 0; i < coda->num_clear-1; i++){
			if(((coda->clear)[i])) free ((coda->clear)[i]);
		}
		if(coda->clear) free(coda->clear);
        if(coda->q)free(coda->q);
        free(coda);
    }
}



/*******************************************************************************
	Funzione che prende una coda (coda), un intero (elem)
	e inserisce elem in coda se possibile, altrimenti aspetta.
*******************************************************************************/
int
insert(queue_t *coda, char *elem)
{
	errno = 0;

    pthread_mutex_lock(&(coda->mtx));
    while(coda->num_el == coda->dim){
        pthread_cond_wait(&(coda->full) , &(coda->mtx));
    }

	/* inserisco l'elemento */
    *(coda->last) = malloc((strlen(elem)+1)*sizeof(char));
    if(!*(coda->last)){
        perror("malloc");
        exit(EXIT_FAILURE);
    }
	strcpy(*(coda->last), elem);
	(coda->num_el)++;

	coda->last =
		( coda->last == (coda->q)+(coda->dim)-1 ) ? coda->q : (coda->last)+1;
    
    pthread_cond_signal(&(coda->empty));
    pthread_mutex_unlock(&(coda->mtx));
	
	return 1;
}


/*******************************************************************************
	Funzione che prende una coda e ne estrae l'elemento in testa
    Se la coda è vuota si mette in attesa che venga riempita da una insert.
*******************************************************************************/
char*
extract(queue_t *coda)
{
	errno = 0;
    pthread_mutex_lock(&(coda->mtx));

	
    while(coda->num_el == 0 && coda->stop == 0){
        pthread_cond_wait(&(coda->empty), &(coda->mtx));
    }
    
	
	if(coda->num_el != 0){

		char *x = *(coda->first);

		(coda->clear)[coda->num_clear-1] = x;
		coda->num_clear++;
		coda->clear = realloc(coda->clear, coda->num_clear * sizeof(char*));
		(coda->clear)[coda->num_clear-1] = NULL;

		(coda->num_el)--;
		coda->first =
			( coda->first == (coda->q)+(coda->dim)-1 ) ? coda->q : (coda->first)+1;

		pthread_cond_signal(&(coda->full));
		pthread_mutex_unlock(&(coda->mtx));
		return x;

	}else if(coda->stop == 1){	
		pthread_mutex_unlock(&(coda->mtx));
		return "stop";
	}

	
}


/*******************************************************************************
	Funzione che prende una coda e restituisce 1 se vuota, 0 altrimenti
*******************************************************************************/
int
empty(queue_t *coda)
{
	return coda->num_el ? 0 : 1;
}


/*******************************************************************************
	Funzione che prende una coda e restituisce 1 se piena, 0 altrimanti
*******************************************************************************/
int
full(queue_t *coda)
{
	return coda->num_el==coda->dim ? 1 : 0;
}