#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
void run_server(struct sockaddr_un *psa);
int collector(char *arg);