
/* Library Imports */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <limits.h>
#include <ftw.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/sendfile.h>

volatile sig_atomic_t connections;

void error(const char *msg)
{
    perror(msg);
    exit(1);
} // Error function used for reporting issues

int main(int argc, char *argv[])
{
    pid_t pid = -5;
    int charsWritten = -1;
    char msg[512];
    int status = -5;
    int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
    socklen_t sizeOfClientInfo;
    char buffer[75000];
    struct sockaddr_in serverAddress, clientAddress;
    connections = 5;
    if (argc < 2)
    {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    } // Check usage & args

    // Set up the address struct for this process (the server)
    memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
    portNumber = atoi(argv[1]);                                  // Get the port number, convert to an integer from a string
    serverAddress.sin_family = AF_INET;                          // Create a network-capable socket
    serverAddress.sin_port = htons(portNumber);                  // Store the port number
    serverAddress.sin_addr.s_addr = INADDR_ANY;                  // Any address is allowed for connection to this process

    // Set up the socket
    listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
    if (listenSocketFD < 0)
        error("ERROR opening socket");

    // Enable the socket to begin listening
    if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
        error("ERROR on binding");

    // Start listening socket
    listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

    while (1)
    {

        // Check if any child process is completed
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
        {
            printf("child %u terminated.\n", (unsigned)pid);
            connections = connections + 1;
            printf("Connection Pool: %d.\n", connections);
        }

        // Control fork count
        if ((connections > 0) && (connections <= 5))
        {

            // printf("Hello"); For Checking fork bomb

            sizeOfClientInfo = sizeof(clientAddress);
            // Get the size of the address for the client that will connect
            establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
            pid = fork();
            connections = connections - 1;

            if (pid == 0)
            {
                // Accept a connection, blocking if one is not available until one connects

                if (establishedConnectionFD < 0)
                    error("ERROR on accept");
                /* Sleep for 2 seconds */
                sleep(2);
                
                /*Step 1: Mode - get/post*/
                memset(buffer, '\0', 512);
                charsRead = recv(establishedConnectionFD, buffer, 255, 0); // Read the client's message from the socket
                if (charsRead < 0)
                    error("ERROR reading from socket");

                memset(msg, '\0', sizeof(msg));
                strcpy(msg, "OK");
                charsWritten = send(establishedConnectionFD, msg, strlen(msg), 0);

                char mode[512];
                memset(mode, '\0', 512);
                strcpy(mode, buffer);

                char user[512];
                memset(user, '\0', 512);

                /*Step 2: Username*/
                memset(buffer, '\0', 512);
                charsRead = recv(establishedConnectionFD, buffer, 255, 0); // Read the client's message from the socket
                strcpy(user, buffer);
                memset(msg, '\0', sizeof(msg));
                strcpy(msg, "OK");
                charsWritten = send(establishedConnectionFD, msg, strlen(msg), 0);

                if (strcmp(mode, "post") == 0)
                {
                    /*Check for file size errors*/
                    memset(msg, '\0', sizeof(msg));
                    recv(establishedConnectionFD, msg, strlen(msg), 0);
                    if (strcmp(msg, "OK"))
                    {
                        send(establishedConnectionFD, msg, sizeof(msg), 0);
                        /*Step 3: File Size of Encrypted Message*/
                        recv(establishedConnectionFD, buffer, sizeof(buffer), 0);
                        int file_size;
                        file_size = atoi(buffer);
                        memset(msg, '\0', sizeof(msg));
                        strcpy(msg, "OK");
                        charsWritten = send(establishedConnectionFD, msg, strlen(msg), 0);
                        /* Step 4: Receive Encrypted Message*/
                        FILE *received_file;
                        int remain_data = 0;
                        ssize_t len;
                        // Dollar expansion for generating file name
                        unsigned int dollar_expansion;
                        dollar_expansion = getpid();
                        char encrypted_file_name[1024];
                        sprintf(encrypted_file_name, "encrypted_%s_%d", user, dollar_expansion);
                        fflush(stdout);
                        fprintf(stdout, "./%s\n", encrypted_file_name);
                        // Receive encrypted file
                        received_file = fopen(encrypted_file_name, "w");
                        if (received_file == NULL)
                        {
                            fprintf(stderr, "D - Failed to open file foo --> %s\n", strerror(errno));
                            exit(EXIT_FAILURE);
                        }

                        remain_data = file_size - 1;
                        int z = 0;
                        memset(buffer, '\0', file_size);
                        while (remain_data > 0)
                        {

                            charsRead = recv(establishedConnectionFD, buffer, sizeof(buffer), 0);

                            if (charsRead > 0)
                            {
                                z = z + 1;
                                fwrite(buffer, sizeof(char), charsRead, received_file);
                                remain_data -= charsRead;
                            }
                        }
                        fclose(received_file);

                        memset(msg, '\0', sizeof(msg));
                        strcpy(msg, "OK");
                        charsWritten = send(establishedConnectionFD, msg, strlen(msg), 0);
                        fflush(stdout);
                    }
                    return 0;
                }
                else if (strcmp(mode, "get") == 0)
                {

                    /* Step 5: Find the oldest encrypted file for the user*/

                    int oldestDirTime = INT_MAX;

                    char targetDirPrefix[2048];
                    sprintf(targetDirPrefix, "encrypted_%s_", user);
                    static char oldestDirName[512];
                    memset(oldestDirName, '\0', sizeof(oldestDirName));
                    DIR *dirToCheck;
                    struct dirent *fileInDir;
                    struct stat dirAttributes;
                    dirToCheck = opendir(".");
                    if (dirToCheck > 0)
                    {
                        while ((fileInDir = readdir(dirToCheck)) != NULL)
                        {
                            if (strstr(fileInDir->d_name, targetDirPrefix) != NULL)
                            {
                                stat(fileInDir->d_name, &dirAttributes);
                                if ((int)dirAttributes.st_mtime < oldestDirTime)
                                {

                                    oldestDirTime = (int)dirAttributes.st_mtime;
                                    memset(oldestDirName, '\0', sizeof(oldestDirTime));
                                    strcpy(oldestDirName, fileInDir->d_name);
                                }
                            }
                        }
                    }
                    closedir(dirToCheck);
                    /* Step 5.01: Read the encryption file*/

                    char ciphertext[sizeof(buffer)];

                    FILE *fp_message = fopen(oldestDirName, "r");
                    memset(msg, '\0', sizeof(msg));
                    strcpy(msg, "OK");
                    if (fp_message == NULL)
                    {
                        strcpy(msg, "No");
                    }
                    send(establishedConnectionFD, msg, sizeof(msg), 0);
                    recv(establishedConnectionFD, msg, sizeof(msg), 0);

                    if (strcmp(msg, "OK") == 0)
                    {
                        fgets(ciphertext, sizeof(ciphertext), fp_message);

                        fclose(fp_message);

                        // /* Step 6: Know the filesize of encrypted file*/
                        struct stat file_stat;
                        int fd;
                        char file_size[512];
                        ssize_t len;
                        fd = open(oldestDirName, O_RDONLY);
                        if (fd == -1)
                        {
                            fprintf(stderr, "Error opening file --> %s", strerror(errno));

                            exit(EXIT_FAILURE);
                        }

                        /* Get file stats */

                        if (fstat(fd, &file_stat) < 0)
                        {
                            fprintf(stderr, "Error fstat --> %s", strerror(errno));

                            exit(EXIT_FAILURE);
                        }

                        sprintf(file_size, "%ld", file_stat.st_size);
                        /* Step 8 : Send file size to client*/
                        len = send(establishedConnectionFD, file_size, sizeof(file_size), 0);
                        if (len < 0)
                        {
                            fprintf(stderr, "Error on sending greetings --> %s", strerror(errno));

                            exit(EXIT_FAILURE);
                        }

                        memset(msg, '\0', sizeof(msg));
                        charsRead = recv(establishedConnectionFD, msg, sizeof(buffer), 0);
                        /* Step 9 : Send the encrypted file to client*/
                        off_t offset = 0;
                        int remain_data = strlen(ciphertext);

                        /* Sending file data */
                        int sent_bytes = 0;
                        while (remain_data > 0)
                        {
                            sent_bytes = send(establishedConnectionFD, ciphertext, strlen(ciphertext), 0);

                            if (sent_bytes > 0)
                            {
                                remain_data -= sent_bytes;
                            }
                        }

                        memset(msg, '\0', sizeof(msg));
                        charsRead = recv(establishedConnectionFD, msg, sizeof(buffer), 0);
                        remove(oldestDirName);

                        fflush(stdout);
                    }
                    /*Closing for Child*/
                    close(establishedConnectionFD); // Close the existing socket which is connected to the client
                    return 0;
                }
            }
            else
            {
                        while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
        {
            printf("child %u terminated.\n", (unsigned)pid);
            connections = connections + 1;
            printf("Connection Pool: %d.\n", connections);
        }
                ; // Not waiting for child
            }
        }
    }
}
