#include "queue.h"
#include <pthread.h>
#include <signal.h>

extern pthread_mutex_t mtx;
extern int fd_skt;
extern int fine;
extern volatile sig_atomic_t stop;
extern queue_t *coda;
extern char **files;
extern char *directory;
extern int dim_files ;
extern int dim;
extern int num_threads;
extern int delay;
extern pid_t pid;