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



#define SOCKNAME "./farm.sck"
#define UNIX_PATH_MAX 108

volatile sig_atomic_t worker;



typedef struct pair{
    long val;
    char *filename;
}pair;

pair **pairs;
int dim_pairs=1;

static void *sigHandler(void *arg){
	sigset_t *set = ((sigset_t*)arg);

	for( ;; ){
		int sig;
		int r = sigwait(set,&sig);

		if(r != 0){
			errno = r;
			perror("sigwait");
			return NULL;
		}

		switch(sig){
            case SIGUSR2:{
                if(worker == 1){

                    for (size_t i = 0; i < dim_pairs; i++)
                    {
                        char *buf = calloc(sizeof(char), 100);
                        sprintf(buf, "%li ", pairs[i]->val);
                        buf = strcat(buf, pairs[i]->filename);
                        buf = strcat(buf, "\n");
                        write(1, buf, strlen(buf));
                        //fprintf(stdout, "%li %s\n", pairs[i]->val, pairs[i]->filename);
                        free(buf);
                    }
                    
                }else {
                    worker--;
                }
            }break;
			default: ;
		}

	}

	return NULL;
}


int aggiorna(fd_set *set, int max){
    for (size_t i = 0; i < max; i++)
    {
        if(!FD_ISSET(i, set)){
            return i-1;
        }
    }
    
}

void run_server(struct sockaddr_un *psa){
    int fd_sk, fd_c, fd_num = 0, fd;
    fd_set set,rdset; int nread;
    fd_sk = socket(AF_UNIX, SOCK_STREAM, 0);
    bind(fd_sk, (struct sockaddr *) psa, sizeof(*psa));
    listen(fd_sk, SOMAXCONN);
    if(fd_sk > fd_num) fd_num = fd_sk;
    FD_ZERO(&set);
    FD_SET(fd_sk, &set);

    while(1){
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
                            close(fd);
                        }else{
                            char* token = strtok(buf, " ");
                            int i=0;
                            int j = 0;
                            pairs[dim_pairs-1] = malloc(sizeof(pair));
                            while(token) {
                                if(i==0){
                                    long val = atol(token);
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
                                        pairs[j]->filename = strcpy(pairs[j]->filename,token);
                                    }
                                    else {
                                        pairs[dim_pairs-1]->filename = malloc(strlen(token)+1);
                                        pairs[dim_pairs-1]->filename = strcpy(pairs[dim_pairs-1]->filename,token);
                                    }
                                }
                                i++;
                                token = strtok(NULL, " ");
                            }

                            dim_pairs++;
                            pairs = realloc(pairs, sizeof(pair*)* dim_pairs);
                        }
                    }
                }
            }
        }
    }
}



int main(int argc, char *argv[]){
    if(argc != 2){
        fprintf(stderr, "usage: %s num_threads\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    worker = atoi(argv[1]);

    sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR2);


	if(pthread_sigmask(SIG_SETMASK,&mask,NULL) != 0){
		fprintf(stderr, "FATAL ERROR\n");
		exit(EXIT_FAILURE);
	}

    pthread_t sighandler_thread;

    if(pthread_create(&sighandler_thread,NULL,sigHandler,&mask) != 0){
		fprintf(stderr, "Error while creating the signal handler thread\n");
        exit(EXIT_FAILURE);
	}

    struct sockaddr_un sa;
    strcpy(sa.sun_path, SOCKNAME);
    sa.sun_family = AF_UNIX;
    pairs = malloc(sizeof(pair*));
    run_server(&sa);
    pairs = realloc(pairs, sizeof(pair)*(dim_pairs-1));
 
        
    if(pthread_join(sighandler_thread, NULL) != 0){
        perror("join");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < dim_pairs-1; i++)
    {
        free(pairs[i]->filename);
        free(pairs[i]);
    }
    free(pairs);

    
    return 0;
}