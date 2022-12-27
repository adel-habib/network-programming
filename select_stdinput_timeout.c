#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define STDIN 0  
#define MILLI 1000

int main(void)
{
	struct timeval tv;
	fd_set readfds;

	tv.tv_sec = 2;
	tv.tv_usec = MILLI * 500;

	FD_ZERO(&readfds);
	FD_SET(STDIN, &readfds);

	select(STDIN+1, &readfds, NULL, NULL, &tv);

	if (FD_ISSET(STDIN, &readfds))
		printf("Something was written to std input [fd num 0].\n");
	else
		printf("Timed out.\n");

	return 0;
}