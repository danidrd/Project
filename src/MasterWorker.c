#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

#include "queue.h"

#define SOCKNAME "./farm"
#define WORKER 4
#define QLEN 8

queue_t *coda;
char **files;
int dim_files = 0;
int dim;
int num_threads;



int show_dir_content( char* path, int depth){
    char buf[PATH_MAX];
    struct stat s;
    char* realpaths;
    int isopen = 0;

    if(depth == 0) realpaths = realpath(path,buf);
    else{
        realpaths = path;
    }

    realpaths[strlen(realpaths)] = '\0';

    DIR* d;

    if(stat(realpaths,&s) == 0){
        if(S_ISDIR(s.st_mode)){
            if(chdir(realpaths) == -1){
                perror("chdir");
                exit(EXIT_FAILURE);
            }
            isopen = 1;
            d = opendir(realpaths);
            
        }else if(S_ISREG(s.st_mode)){
            dim_files++;
            files = realloc(files, dim_files * sizeof(char*));

            if(!files){
                perror("realloc");
                exit(EXIT_FAILURE);
            }
            files[dim_files -1] = calloc((strlen(path) + 1), sizeof(char));
            if(!files[dim_files - 1]){
                perror("malloc");
                exit(EXIT_FAILURE);
            }
            strcpy(files[dim_files -1], path);
            

            return 1;     
        }

        if(!isopen) d = opendir(realpaths);
        struct dirent* dir;

        while((errno = 0, dir = readdir(d)) != NULL){
            realpaths = realpath(dir->d_name,buf);

            if(!strcmp(dir->d_name,".") || !strcmp(dir->d_name,"..")){
                continue;
            }

            if(dir->d_type == DT_DIR){
                if((show_dir_content(realpaths,1)) == 1){
                    if(chdir("..") == -1){
                        perror("chdir");
                        exit(EXIT_FAILURE);
                    }
                }
            }else if(dir->d_type == DT_REG){
                show_dir_content(realpaths,1);

            }

        }

        if(closedir(d) == -1){
            perror("closing cwd");
            exit(EXIT_FAILURE);
        }

        return 1;
    }

    return 0;


}

static void * master(void * arg){
    char **file_names = (char**)&arg;
    
    if(dim == 1){
        
        show_dir_content(file_names[0], 0);

        for (size_t i = 0; i < dim_files; i++)
        {
            insert(coda, files[i]);
        }

    }else {
        for (size_t i = 0; i < dim; i++)
        {
            insert(coda, file_names[i]);
        }
        
    }
    
    
    pthread_mutex_lock(&coda->mtx);
    coda->stop = 1;
    pthread_cond_broadcast(&coda->empty);
    pthread_mutex_unlock(&coda->mtx);
    
    

    

}

static void * worker(void *arg){

    char *str;
    do{
        str = extract(coda);
        printf("%s\n",str);
    }while(strcmp(str, "stop") != 0);
    
    

}

int main(int argc, char *argv[]){

    if(argc < 2){
        fprintf(stderr,"Usage: %s [-n -q -d -t] [list of files >= 0]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int opt,err;
    struct stat s;
    int h,n = 0,q = 0;
    char *d=NULL;
    double t;
    DIR *dir;
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
                fprintf(stdout,"%d\n",n);
            } break;

            case 'q':{
                if((q = atoi(optarg)) == 0){
                    fprintf(stderr,"Error on atoi function\n");
                    exit(EXIT_FAILURE);
                }

                fprintf(stdout,"%d\n",q);
            }break;

            case 'd':{
                d = optarg;
                if(stat(d,&s) == 0){
                    if(!S_ISDIR(s.st_mode)){
                        fprintf(stderr,"The given string isn't a directory name\n");
                        exit(EXIT_FAILURE);
                    }
                    dir = opendir(d);
                    free(dir);
                }else {
                    perror("stat");
                }

                fprintf(stdout,"%s\n", d);
            }break;

            case 't':{
                t = atof(optarg);
                fprintf(stdout,"%.2lf\n", t);
            }break;
        }
    }
    char **arg;
    int dim_arg = argc - optind;
    if(dim_arg > 0){
        dim = dim_arg;
        arg = malloc(sizeof(char*) * dim_arg);
        if(!arg){
            perror("malloc");
            exit(EXIT_FAILURE);
        }
    }
    if(d == NULL){

        for (size_t i = optind; i < argc; i++)
        {
            if(stat(argv[i],&s) == 0){
                if(!S_ISREG(s.st_mode)){
                    fprintf(stderr,"The given string isn't a regular file name\n");
                    exit(EXIT_FAILURE);
                }
                arg[i-optind] = malloc((strlen(argv[i])+1) * sizeof(char));
                if(!arg[i-optind]){
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
                strcpy(arg[i-optind], argv[i]);
            }else{
                perror("stat");
                exit(EXIT_FAILURE);
            }
            
        }

    }

    if(dim_arg == 0){
        dim = 1;
        arg = malloc(sizeof(char*));
        if(!arg){
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        arg[0] = malloc(sizeof(char)* (strlen(d) + 1));
        if(!arg[0]){
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        strcpy(arg[0], d);
    }
    

    if(n == 0){
        tidw = malloc(sizeof(pthread_t) * WORKER);
        num_threads = WORKER;
        if(!tidw){
            perror("malloc");
            exit(EXIT_FAILURE);
        }
    }
    
    
    

    if(( err = pthread_create(&tidm, NULL, &master, (void*) *arg)) != 0){
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
    
    if(q != 0){
        coda = new_queue(q);
    }else{
        coda = new_queue(QLEN);
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

    for (size_t i = 0; i < dim_arg; i++)
    {
        free(arg[i]);
    }
    if(dim_arg == 0){
        free(arg[0]);
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