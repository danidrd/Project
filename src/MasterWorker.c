#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>

//#include "queue.h"
#include "master.h"
#include "worker.h"
#include "MasterWorker.h"

#define SOCKNAME "./farm.sck"
#define WORKER 4
#define QLEN 8

#define SYSCALL(r,c,e)\
	if((r=c)==-1){perror(e);exit(errno); }

typedef struct master_args{
    queue_t *queue; // OK
    char **files; // OK
    char *directory; // OK
    int dim_files; // OK
    int delay; // OK
    pid_t pid; // OK
}m_args;

typedef struct worker_args{
    queue_t *queue; // OK
    char *directory; // OK
    pid_t pid; // OK
}w_args;



queue_t *coda;
char **files;
char *directory;
int dim_files = 0;
int dim;
int num_threads;
int delay = 0;
pid_t pid;


void cleanup(){
    unlink(SOCKNAME);
}







int main(int argc, char *argv[]){

    if(argc < 2){
        fprintf(stderr,"Usage: %s [-n -q -d -t] [list of files >= 0]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    cleanup();
    int opt,err;
    struct stat s;
    int h,n = 0,q = 0;
    char *d=NULL;
    int t;
    pthread_t tidm, *tidw;

    for(h=1;h<argc;h++){
        if(argv[h][0] == '-'){
            if(argv[h][1]){

                char letter = argv[h][1];

                if(letter == 'h'){
                    fprintf(stdout, "%s -n <number-thread> -q <queue-length> -d <directory-name> -t <delay> -h help\n", argv[0]);
                    return 0;
                }
            }
        }
    }

    h = 1;
    for(h=1; h < argc; h++){
        if(argv[h][0] == '-'){
            h++;
            continue;
        }else{
            char buf[PATH_MAX];
            char* realpaths;

            realpaths = realpath(argv[h], buf);
            realpaths[strlen(realpaths)] = '\0';
            

            dim_files++;
            files = realloc(files, dim_files * sizeof(char*));

            if(!files){
                perror("realloc");
                exit(EXIT_FAILURE);
            }
            files[dim_files -1] = calloc((strlen(realpaths) + 1), sizeof(char));
            if(!files[dim_files - 1]){
                perror("malloc");
                exit(EXIT_FAILURE);
            }
            strcpy(files[dim_files -1], realpaths);
        }
    }


    while((opt = getopt(argc,argv,"n:q:d:t:h")) != -1){

        switch(opt){

            case '?':{
                printf("Errore : parametro %c non riconosciuto\n", opt);
                return -1;
            } break;

            case 'n':{
                if((n = atoi(optarg)) == 0){
                    fprintf(stderr,"Error on atoi function\n");
                    exit(EXIT_FAILURE);
                }
                tidw = malloc(sizeof(pthread_t) * n);
                num_threads = n;
                if(!tidw){
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
            } break;

            case 'q':{
                if((q = atoi(optarg)) == 0){
                    fprintf(stderr,"Error on atoi function\n");
                    exit(EXIT_FAILURE);
                }

            }break;

            case 'd':{
                d = optarg;
                if(stat(d,&s) == 0){
                    if(!S_ISDIR(s.st_mode)){
                        fprintf(stderr,"The given string isn't a directory name\n");
                        exit(EXIT_FAILURE);
                    }

                directory = d;
                
                }else {
                    perror("stat");
                }

            }break;

            case 't':{
                if((t = atoi(optarg)) == 0){
                    fprintf(stderr,"Error on atoi function\n");
                    exit(EXIT_FAILURE);
                }
                delay = t;
            }break;
        }
    }

    

    if(n == 0){
        tidw = malloc(sizeof(pthread_t) * WORKER);
        num_threads = WORKER;
        if(!tidw){
            perror("malloc");
            exit(EXIT_FAILURE);
        }
    }
    
    
    if(q != 0){
        coda = new_queue(q);
    }else{
        coda = new_queue(QLEN);
    }


    pid = fork();
    if(pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    char *arg = calloc(sizeof(char), 100);
    sprintf(arg,"%d",num_threads);
    if(pid == 0){
        pid = getpid();
        execl("/home/daniele/Scrivania/SOL_ASSIGNMENT/Project/bin/collector", "collector",arg, (char*) NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    }


    if(( err = pthread_create(&tidm, NULL, &master, (void*) NULL)) != 0){
        fprintf(stderr,"Error while creating thread master\n");
        exit(EXIT_FAILURE);
    }

    if(n != 0){

        for (size_t j = 0; j < n; j++)
        {
            if((err = pthread_create(&tidw[j], NULL, &worker, (void*) NULL)) != 0){
                fprintf(stderr, "Error while creating thread worker\n");
                exit(EXIT_FAILURE);
            }
        }

    }else{

        for (size_t j = 0; j < WORKER; j++)
        {
            if((err = pthread_create(&tidw[j], NULL, &worker, (void*) NULL)) != 0){
                fprintf(stderr, "Error while creating thread worker\n");
                exit(EXIT_FAILURE);
            }
        }
        
    }
    




    if(pthread_join(tidm, NULL) != 0){
        perror("pthread_join");
        exit(EXIT_FAILURE);
    }

    if(n != 0){

        for (size_t j = 0; j < n; j++)
        {
            if(pthread_join(tidw[j], NULL) != 0){
                perror("pthread_join");
                exit(EXIT_FAILURE);
            }
        }
    }else{

        for (size_t j = 0; j < WORKER; j++)
        {
            if(pthread_join(tidw[j], NULL) != 0){
                perror("pthread_join");
                exit(EXIT_FAILURE);
            }
        }

    }

    for (size_t i = 0; i < dim_files; i++)
    {
        free(files[i]);
    }

    free(files);
    free(arg);
    
    delete_queue(coda);
    free(tidw);





}