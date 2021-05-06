#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main()
{
    pid_t pid;
    int status;
    printf("Hello world\n");
    status = waitpid(-1, NULL, WNOHANG);
    int i=0;
    for (i=1;i<5;i++)
    {
        while (status > 0)
        {
            write(1, "Hello", 6);
        }
        pid = fork();
        if (pid = 0)
        {
            printf("I am child\n");
            sleep(10);
        }
        else
        {
            ;
        }
    }
}