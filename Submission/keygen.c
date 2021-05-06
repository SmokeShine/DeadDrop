/*
 ============================================================================
 Name        : keygen.c
 Author      : Prateek Gupta
 Version     : v1
 ============================================================================
 */

/*Usage
command arg1  [> output_file]
Items in brackets are optional
*/

/* Test Script Results Sample
$ ./keygen 30
LLQLVFXCQLALLKOUVRXWBBEDAPRLON
$ ./keygen 30 > key30
$ wc -c key30
31 key30
*/

/*library imports*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

char* getRandom(int lower, int upper,int limit)
{
    /*Selects randoms number between the upper and lower limit*/
    /* If the selected number is ascii for @, convert it to space*/
    /*Returns pointer*/
    /*Random Seed*/
    srand ( time(NULL) );
    /*Variables on Stack*/
    int i;
    int num;
    char selected[limit];
    char *pointer;
    /*allocating space in heap, pointer will point to this address*/
    pointer=(char *)malloc(limit*sizeof(char)); 
    
    for (i=0;i<limit;i++)
    {
        /*Selecting a random number*/
        num = (rand() % (upper - lower + 1)) + lower;
        /*Hack for showing spaces*/
        /*Check for ascii for @*/
        if (num==64)
        {
            /*Replace with ascii for space*/
            num=32;
        }
        /*Type casting it to ascii value*/
        pointer[i] = (char)num;
    }
    /*Returns pointer to the heap*/
    return pointer;
}

int main(int argc, char *argv[])
{
    /*Generate 10 alphabets with 1 newline character*/
    char * p;
    int i;
    int limit;
    /*Argument length check*/
    if (argc<2)
    {
        /*If length is not provides, print to stderr*/
        fprintf(stderr,"Length not provided\n");
        /*Return failure*/
        exit(EXIT_FAILURE);
    }
    // Convert argument length string to an integer
    limit =atoi(argv[1]); 
    // Get pointer storing random numbers on heap
    p=getRandom(64,90,limit);
    for ( i =0; i<limit;i++)
    {
        /*print to screen*/
        printf("%c",*p);
        p++;
    }
    /*Add a new line character to convert to valid string*/
    printf("\n");
    return 0;
}
