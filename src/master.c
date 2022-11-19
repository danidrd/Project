#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>

#include "master.h"
#include "MasterWorker.h"

#define UNIX_PATH_MAX 108
#define SOCKNAME "./farm.sck"

#define SYSCALL(r,c,e)\
	if((r=c)==-1){perror(e);exit(errno); }


int fd_skt;



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

void * master(void * arg){

    struct timespec ts;
    int res;

    int err; 
    struct sockaddr_un sa;
	strncpy(sa.sun_path, SOCKNAME, UNIX_PATH_MAX);
	sa.sun_family=AF_UNIX;

	SYSCALL(fd_skt,socket(AF_UNIX,SOCK_STREAM,0),"creating socket");
	while(connect(fd_skt, (struct sockaddr*) &sa, sizeof(sa)) == -1){
		if(errno == ENOENT) sleep(1);
		else exit(EXIT_FAILURE);
	}

    if(delay < 0){
        fprintf(stderr,"delay cannot be less than 0");
        exit(EXIT_FAILURE);
    }

    ts.tv_sec = delay/1000;
    ts.tv_nsec = (delay % 1000) * 1000000;
    if(directory) show_dir_content(directory, 0);
    for (size_t i = 0; i < dim_files; i++)
    {
        if(i != 0){
            do{
                res = nanosleep(&ts, &ts);
            }while(res && errno == EINTR);
        }

        insert(coda, files[i]);
    }

    
    
    pthread_mutex_lock(&coda->mtx);
    coda->stop = 1;
    pthread_cond_broadcast(&coda->empty);
    pthread_mutex_unlock(&coda->mtx);
    
    

    

}