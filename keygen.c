#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

char* getRandom(int lower, int upper,int limit)
{
    srand ( time(NULL) );
    /*Variables on Stack*/
    int i;
    int num;
    char selected[limit];
    char *pointer;
    pointer=(char *)malloc(limit*sizeof(char)); /*allocating space in heap, pointer will point to this address*/
    
    for (i=0;i<limit;i++)
    {
        num = (rand() % (upper - lower + 1)) + lower;
        /*Check for duplicate*/
        if (num==64)
        {
            num=32;
        }
        pointer[i] = (char)num;/*Type casting it to ascii value*/
    }
    
    return pointer;
}

int main(int argc, char *argv[])
{
    /*Generate 10 alphabets with 1 newline character*/
    char * p;
    int i;
    int limit;
    if (argc<2)
    {
        printf("Length not provided");
        exit(EXIT_FAILURE);
    }
    limit =atoi(argv[1]);
    p=getRandom(64,90,limit);
    for ( i =0; i<limit;i++)
    {
        printf("%c",*p);
        p++;
    }
    printf("\n");
    return 0;
}
