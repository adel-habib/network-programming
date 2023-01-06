#include <stdio.h>
#include <poll.h>

// this program basically detects the "event" of the stdinput fd being ready for a read syscall or timesout after 2.5 seconds
int main(void)
{
    // std input is file descriptor number 0

    struct pollfd pfds[1]; 

    pfds[0].fd = 0;          // Standard input
    pfds[0].events = POLLIN; // Tell me when ready to read


    printf("Hit RETURN or wait 2.5 seconds for timeout\n");

    // poll only returns the number of fds ready for event, it doesn't tell us which ones are ready for read
    int num_events = poll(pfds, 1, 2500); 

    if (num_events == 0)
    {
        printf("Poll timed out!\n");
    }
    else
    {
        // we have to check manually for each fds and event
        int pollin_happened = pfds[0].revents & POLLIN;

        if (pollin_happened)
        {
            printf("File descriptor %d is ready to read\n", pfds[0].fd);
        }
        else
        {
            printf("Unexpected event occurred: %d\n", pfds[0].revents);
        }
    }

    return 0;
}