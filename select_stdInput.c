#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#define STDIN 0 
// a bad dead simple example to hammer the mechanism of select in. 
int main() {

   fd_set fds;
   FD_ZERO(&fds);
   FD_SET(STDIN,&fds);
   char buff[256];
   while(1) {

    select(1,&fds,NULL,NULL,NULL);

    if(FD_ISSET(STDIN,&fds)) {
        printf("SOCKET READY TO READ\n");
        int nbytes = read(STDIN,buff,sizeof(buff));
        printf("READ %d bytes as: %s\n",nbytes,buff);
        return 0;
    }

   }
}
