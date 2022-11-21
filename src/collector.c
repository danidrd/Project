#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>


#include "collector.h"

#define SOCKNAME "farm.sck"
#define UNIX_PATH_MAX 108

#define ec_meno1(s,m)\
	if( (s) == -1){perror(m);exit(EXIT_FAILURE);}

int workers; //Variabile che rappresenta il numero di worker presenti nel processo farm, appena è 1 il processo collector stampa i risultati


//Struttura usata dalla funzione run_server per implementare un array di coppie <valore, filename>
typedef struct pair{
    long val;
    char *filename;
}pair;

pair **pairs; //Array di coppie
int dim_pairs=1; //Dimensione dell'array pairs

//Funzione che restituisce l'ultimo file descriptor settato nel fd_set 
int aggiorna(fd_set *set, int max){
    for (size_t i = 0; i < max; i++)
    {
        if(!FD_ISSET(i, set)){
            return i-1;
        }
    }
    
}
//Funzione che implementa la gestione dei file descriptor tramite la select, riceve le stringhe "valore filename" dai worker
//e le inserisce in modo ordinato nella struttura pair
void run_server(struct sockaddr_un *psa){
    int fd_sk, fd_c, fd_num = 0, fd;
    fd_set set,rdset; int nread;
    fd_sk = socket(AF_UNIX, SOCK_STREAM, 0);
    bind(fd_sk, (struct sockaddr *) psa, sizeof(*psa));
    listen(fd_sk, SOMAXCONN);
    if(fd_sk > fd_num) fd_num = fd_sk;
    FD_ZERO(&set);
    FD_SET(fd_sk, &set);

    while(workers != 0){
        rdset = set;
        if(select(fd_num +1, &rdset, NULL, NULL, NULL) == -1){
            perror("select");
            exit(EXIT_FAILURE);
        }else{
            for(fd = 0; fd<=fd_num;fd++){
                if(FD_ISSET(fd, &rdset)){
                    if(fd == fd_sk){
                        fd_c = accept(fd_sk, NULL, 0);
                        FD_SET(fd_c, &set);
                        if(fd_c > fd_num) fd_num = fd_c;

                    }else{

                        char buf[100];
                        nread = read(fd,buf,100);
                        if(nread==0){
                            FD_CLR(fd, &set);
                            fd_num = aggiorna(&set, fd_num);
                            workers--;
                            close(fd);
                        }else{
                            if(strncmp(buf,"print",5) == 0){

                                for (size_t i = 0; i < dim_pairs-1; i++)
                                {
                                    char *buf2 = calloc(sizeof(char), 100);
                                    if(!buf2){
                                        perror("calloc");
                                        exit(EXIT_FAILURE);
                                    }
                                    sprintf(buf2, "%li ", pairs[i]->val);
                                    buf2 = strcat(buf2, pairs[i]->filename);
                                    buf2 = strcat(buf2, "\n");
                                    if(write(1, buf2, strlen(buf2)) == -1){
                                        perror("malloc");
                                        exit(EXIT_FAILURE);
                                    }
                                    free(buf2);
                                    
                                }
                                continue;

                            }
                            char* token = strtok(buf, " ");
                            int i=0;
                            int j = 0;
                            pairs[dim_pairs-1] = malloc(sizeof(pair));
                            if(!pairs[dim_pairs-1]){
                                perror("malloc");
                                exit(EXIT_FAILURE);
                            }
                            while(token) {
                                if(i==0){
                                    long val = atol(token);
                                    if(val == 0){
                                        fprintf(stderr, "Error on atol function\n");
                                        exit(EXIT_FAILURE);
                                    }
                                    for (j = 0; j < dim_pairs; j++)
                                    {
                                        if(j==dim_pairs-1)break;
                                        if(pairs[j]->val >= val){
                                            break;
                                        }
                                    }
                                    if(j != dim_pairs-1) {
                                        for (size_t k = dim_pairs-1; k >= j; k--)
                                        {
                                            pairs[k]->val = pairs[k-1]->val;
                                        }
                                        
                                        pairs[j]->val = val;
                                    }
                                    else pairs[dim_pairs-1]->val = val;
                                }else {
                                    if(j != dim_pairs-1) {
                                        for (size_t k = dim_pairs-1; k >= j; k--)
                                        {
                                            pairs[k]->filename = pairs[k-1]->filename;
                                        }

                                        pairs[j]->filename = malloc(strlen(token)+1);
                                        if(!pairs[j]->filename){
                                            perror("malloc");
                                            exit(EXIT_FAILURE);
                                        }
                                        pairs[j]->filename = strcpy(pairs[j]->filename,token);
                                    }
                                    else {
                                        pairs[dim_pairs-1]->filename = malloc(strlen(token)+1);
                                        if(!pairs[dim_pairs-1]){
                                            perror("malloc");
                                            exit(EXIT_FAILURE);
                                        }
                                        pairs[dim_pairs-1]->filename = strcpy(pairs[dim_pairs-1]->filename,token);
                                    }
                                }
                                i++;
                                token = strtok(NULL, " ");
                            }

                            dim_pairs++;
                            pairs = realloc(pairs, sizeof(pair*)* dim_pairs);
                            if(!pairs){
                                perror("malloc");
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                }
            }
        }
    }
    close(fd_sk);
    return;
}



int collector(char *arg){
    workers = 1;

    struct sigaction s;
	memset( &s, 0, sizeof(s));
	s.sa_handler=SIG_IGN;
	ec_meno1(sigaction(SIGINT,&s,NULL),"sigaction");
	ec_meno1(sigaction(SIGQUIT,&s,NULL),"sigaction");
    ec_meno1(sigaction(SIGHUP,&s,NULL),"sigaction");
	ec_meno1(sigaction(SIGTERM,&s,NULL),"sigaction");
	ec_meno1(sigaction(SIGUSR1,&s,NULL),"sigaction");

	ec_meno1(sigaction(SIGPIPE,&s,NULL),"sigaction");






    struct sockaddr_un sa;
    strcpy(sa.sun_path, SOCKNAME);
    sa.sun_family = AF_UNIX;
    pairs = malloc(sizeof(pair*));
    if(!pairs){
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    run_server(&sa);
    pairs = realloc(pairs, sizeof(pair)*(dim_pairs-1));
    if(!pairs){
        perror("realloc");
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < dim_pairs-1; i++)
    {
        char *buf = calloc(sizeof(char), 100);
        if(!buf){
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        sprintf(buf, "%li ", pairs[i]->val);
        buf = strcat(buf, pairs[i]->filename);
        buf = strcat(buf, "\n");
        if(write(1, buf, strlen(buf)) == -1){
            perror("write");
            exit(EXIT_FAILURE);
        }
        free(buf);
    }


    for (size_t i = 0; i < dim_pairs-1; i++)
    {
        free(pairs[i]->filename);
        free(pairs[i]);
    }
    free(pairs);
    return 0;
}