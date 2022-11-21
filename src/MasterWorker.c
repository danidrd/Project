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
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>

#include "master.h"
#include "worker.h"
#include "MasterWorker.h"
#include "collector.h"

#define SOCKNAME "./farm.sck"
#define WORKER 4
#define QLEN 8
#define MAX_PATH 255

#define SYSCALL(r,c,e)\
	if((r=c)==-1){perror(e);exit(errno); }



int fine = 0;   //Variabile globale usata per terminare il sigHandler ed i Worker thread
queue_t *coda;  //Variabile queue_t globale condivisa tra i thread Worker e Master per l'inserimento ed estrazione in coda
char **files;   //Array di stringhe contenente i nomi di tutti i file passati come argomento
char *directory;    //Stringa contenente il nome della directory (Usata dai worker per determinare il path relativo di ogni file)
int dim_files = 0; //Dimensione dell'array di stringhe files
int num_threads;     //Numero di thread worker
int delay = 0;  //Intero che rappresenta il ritardo con cui inviare due richieste successive
pid_t pid;  //Process id del processo figlio (Collector)
volatile sig_atomic_t stop = 0; //Variabile usata per determinare la fine del sigHandler e del thread master
int fd_skt; //Variabile condivisa dal thread master e workers 
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; //MUTEX CONDIVISA

//Funzione che cancella il file di nome path dal file system
void cleanup(char *path){
    
    if(unlink(path)!= 0) {
        perror("unlink");
        exit(EXIT_FAILURE);
    }
}


//Funzione che attende la ricezione di un segnale, passata come argomento di una pthrea_create nel main
void *sigHandler(void *arg){
	sigset_t *set = ((sigset_t*)arg);

	while(fine!=num_threads && stop == 0){

        struct timespec ts;
        ts.tv_sec = 1;
        ts.tv_nsec = 0;
		int sig;
		sig = sigtimedwait(set,NULL, &ts);

		if(sig == -1){
			if(errno == EAGAIN) continue;
			perror("sigwait");
			return NULL;
		}

		switch(sig){
            case SIGHUP:{
              
              stop = 1;
                
            }break;
            case SIGINT:{
              
              stop = 1;
                
            }break;
            case SIGQUIT:{
              
              stop = 1;
                
            }break;
            case SIGTERM:{
              
              stop = 1;
                
            }break;
            case SIGUSR1:{
              //Notifica il processo pid di stampare i file elaborati sino a quel momento
            int err;
            pthread_mutex_lock(&mtx);
            SYSCALL(err,write(fd_skt,"print",5),"write client");
            pthread_mutex_unlock(&mtx);
            
                
            }break;
            case SIGPIPE:{
              
              fprintf(stderr, "SIGPIPE\n");
              exit(EXIT_FAILURE);
                
            }break;

			default: ;
		}

	}
	return NULL;
}






int main(int argc, char *argv[]){

    if(argc < 2){
        fprintf(stderr,"Usage: %s [-n -q -d -t] [list of files >= 0]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char buf[PATH_MAX];
    char* realpaths;
    realpaths = realpath(SOCKNAME,buf);
    int opt,err;
    struct stat s;
    int h,n = 0,q = 0;
    char *d=NULL;
    int t;
    pthread_t tidm, *tidw, sighandler;

    sigset_t mask;
	sigemptyset(&mask);
	if(sigaddset(&mask, SIGUSR1)==-1){
        perror("sigaddset");
        exit(EXIT_FAILURE);
    }
    if(sigaddset(&mask, SIGTERM) == -1){
        perror("sigaddset");
        exit(EXIT_FAILURE);
    }
    if(sigaddset(&mask, SIGQUIT) == -1){
        perror("sigaddset");
        exit(EXIT_FAILURE);
    }
    if(sigaddset(&mask, SIGINT) == -1){
        perror("sigaddset");
        exit(EXIT_FAILURE);
    }
    if(sigaddset(&mask, SIGHUP) == -1){
        perror("sigaddset");
        exit(EXIT_FAILURE);
    }
    int i= 0;
    if(pthread_sigmask(SIG_SETMASK,&mask,NULL) != 0){
		fprintf(stderr, "FATAL ERROR\n");
		exit(EXIT_FAILURE);
	}






	
    
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
    //Ciclo che inserisce in files il path assoluto dei file contenuti in argv
    for(h=1; h < argc; h++){
        if(argv[h][0] == '-'){
            h++;
            continue;
        }else{
            char buf[MAX_PATH];
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
            if(strcpy(files[dim_files -1], realpaths) == NULL){
                fprintf(stderr, "Error on strcpy\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    //Ciclo per gestire le opzioni del programma
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

    
    //Se l'argomento -n non è stato specificato, si allocano WORKER tid 
    if(n == 0){
        tidw = malloc(sizeof(pthread_t) * WORKER);
        num_threads = WORKER;
        if(!tidw){
            perror("malloc");
            exit(EXIT_FAILURE);
        }
    }else{
        tidw = malloc(sizeof(pthread_t) * n);
        num_threads = n;
        if(!tidw){
            perror("malloc");
            exit(EXIT_FAILURE);
        }
    }
    
    //Se l'argomento q è stato usato, si alloca una coda di dimensione fissa q, altrimenti si usa QLEN di default
    if(q != 0){
        coda = new_queue(q);
    }else{
        coda = new_queue(QLEN);
    }


    char *arg = calloc(sizeof(char), 100);
    if(sprintf(arg,"%d",num_threads) < 0){
        fprintf(stderr, "Error on sprintf\n");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if(pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if(pid == 0){
        pid = getpid();
        collector(arg);
        exit(EXIT_SUCCESS);
    }


    if(pthread_create(&sighandler,NULL,sigHandler,&mask) != 0){
		fprintf(stderr, "Error while creating the signal handler thread\n");
        exit(EXIT_FAILURE);
	}

    if(( err = pthread_create(&tidm, NULL, &master, (void*) NULL)) != 0){
        fprintf(stderr,"Error while creating thread master\n");
        exit(EXIT_FAILURE);
    }

 

    for (size_t j = 0; j < num_threads; j++)
    {
        if((err = pthread_create(&tidw[j], NULL, &worker, (void*) NULL)) != 0){
            fprintf(stderr, "Error while creating thread worker\n");
            exit(EXIT_FAILURE);
        }
    }


    


    if(pthread_join(sighandler, NULL) != 0){
        perror("join");
        exit(EXIT_FAILURE);
    }


    if(pthread_join(tidm, NULL) != 0){
        perror("pthread_join");
        exit(EXIT_FAILURE);
    }


    for (size_t j = 0; j < num_threads; j++)
    {
        if(pthread_join(tidw[j], NULL) != 0){
            perror("pthread_join");
            exit(EXIT_FAILURE);
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
    int status;
    if((waitpid(pid, &status, 0)) == -1){
        perror("waitpid");
        exit(EXIT_FAILURE);
    }
    cleanup(buf);
    exit(EXIT_SUCCESS);

}