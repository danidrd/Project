#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>

#include "worker.h"
#include "master.h"
#include "MasterWorker.h"


#define SYSCALL(r,c,e)\
	if((r=c)==-1){perror(e);exit(errno); }



static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

void * worker(void *arg){



    char *str;
    while(1){
        
        str = extract(coda);
        if(strcmp(str,"stop") != 0){
            FILE *ifp;
            ifp = fopen(str, "rb");
            char *path = strstr(str, directory);

            long content;
            long result = 0;
            long i = 0;
            if(!ifp) continue;
            int bytes;
          
            while((bytes = fread((void*)&content, sizeof(long), 1, ifp)) > 0){
                if(feof(ifp)){break;}
                if(ferror(ifp)){
                    perror("fread");
                    exit(EXIT_FAILURE);
                }
                

                result += (i*content);
                i++;

            }
            char *last = strrchr(str,'/');
            char *writ = calloc(sizeof(char),100);
            sprintf(writ, "%li", result);
            if(path){
                writ = strcat(writ, " ");
                writ = strcat(writ, path);
            }else{
                writ = strcat(writ, " ");
                writ = strcat(writ,last+1);
            }
            int err;
            pthread_mutex_lock(&mtx);
            SYSCALL(err,write(fd_skt,writ,100),"write client");
            pthread_mutex_unlock(&mtx);
            free(writ);
            fclose(ifp);
            
        }else break;

    }
    pthread_mutex_lock(&mtx);
    kill(pid, 12);
    pthread_mutex_unlock(&mtx);
    

}