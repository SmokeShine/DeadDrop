#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int convert_to_index(char s)
{
    if (s==' ')
    {
        return 0;
    }
    else
    {
        return (int)s-65;
    }
}

char convert_to_char(int i)
{
    if (i==0)
    {
        return 32;
    }
    else
    {
        return (char)i+65;
    }
}

int main()
{
    printf("Hello world\n");
    /*Encryption Step*/
    int i = 0;
    char *message;
    char *key;
    size_t bufsize = 512;
    size_t message_length;
    size_t key_length;
    message = (char *)malloc(bufsize * sizeof(char));
    message_length = getline(&message, &bufsize, stdin);

    key = (char *)malloc(bufsize * sizeof(char));
    key_length = getline(&key, &bufsize, stdin);

    if (message_length != key_length)
    {
        printf("Mismatch\n");
        exit(EXIT_FAILURE);
    }

    /*Fixing Case*/
    for (i = 0; i <= strlen(message); i++)
    {
        if (message[i] >= 97 && message[i] <= 122)
            message[i] = message[i] - 32;
    }

    for (i = 0; i <= strlen(key); i++)
    {
        if (key[i] >= 97 && key[i] <= 122)
            key[i] = key[i] - 32;
    }

    /*Add both of them - ascii value?*/
    char *message_and_key = (char *)malloc(bufsize * sizeof(char));
    char *ciphertext = (char *)malloc(bufsize * sizeof(char));
    int temp=0;
    int temp_message_key=0;
    for (i = 0; i < message_length-1; i++)/*Excluding null at the end*/
    {
        temp = convert_to_index(message[i]) + convert_to_index(key[i]);
        temp_message_key = ((temp % 27)+27)%27;
        ciphertext[i]=convert_to_char(temp_message_key);
    }
    printf("%s\n", ciphertext);

    /*Decrypt*/
    char *solution = (char *)malloc(bufsize * sizeof(char));
    for (i = 0; i < message_length-1; i++)/*Excluding null at the end*/
    {
        temp=ciphertext[i]-key[i];
        temp_message_key = ((temp % 27)+27)%27;
        printf("%d \t",temp_message_key);
        solution[i]=convert_to_char(temp_message_key);
    }
    printf("%s\n", solution);

    return 0;
}