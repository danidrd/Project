#include "queue.h"
#include <pthread.h>

extern queue_t *coda;
extern char **files;
extern char *directory;
extern int dim_files ;
extern int dim;
extern int num_threads;
extern int delay;
extern pid_t pid;