/*
 ============================================================================
 Name        : otp_d.c
 Author      : Prateek Gupta
 Version     : v1
 ============================================================================
 */

/*Usage
otp_d listening_port
listening_port is the port that otp_d should listen on. 
always start otp_d in the background and ps after each call from client to check for fork bombs
*/

/* Test Script Results Sample
$ otp_d 5555 &
$ otp_d 5555 
*/

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

/*atomic entity accessible even in the presence of asynchronous interrupts made by signals.*/
/*Variable to track number of pool connections*/
volatile sig_atomic_t connections;

/*Error handlers for socket connections*/
void error(const char *msg)
{
    // Error function used for reporting issues
    perror(msg);
    exit(0);
}

/*Signal handler to sigchld*/
void my_sigchld_handler(int sig)
{
    pid_t p;
    int status;
    /*Check if any child has completed process*/
    while ((p = waitpid(-1, &status, WNOHANG)) > 0)
    {
        /* increase the connection pool: Allow more clients to connect */
        connections = connections + 1;
    }
}

int main(int argc, char *argv[])
{
    /*Helper functions*/
    pid_t pid = -5;
    int charsWritten = -1;
    char msg[512];
    int status = -5;
    int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
    socklen_t sizeOfClientInfo;
    char buffer[75000];
    struct sockaddr_in serverAddress, clientAddress;
    /*Connection pool. Edit to allow more connections. Performance hit on increase*/
    connections = 5;
    /*When a child process stops or terminates, SIGCHLD is sent to the parent process. The default response to the signal is to ignore it*/
    /*We are modifying the default action to manage the connection pool count*/
    struct sigaction SIGINT_handler_struct_parent = {0};
    sigset_t block_mask_parent;
    sigaddset(&block_mask_parent, SIGCHLD);
    sigaddset(&block_mask_parent, SIGQUIT);
    /*
    struct sigaction {
    void (*sa_handler)(int); - Pointer to a signal-catching function or one of the macros SIG_IGN or SIG_DFL.
    sigset_t sa_mask; - Additional set of signals to be blocked during execution of signal-catching function.
    int sa_flags; - Special flags to affect behavior of signal.
    void (*sa_restorer)(void); - old, Pointer to a signal-catching function.
    }
    */
    SIGINT_handler_struct_parent.sa_handler = my_sigchld_handler;
    SIGINT_handler_struct_parent.sa_mask = block_mask_parent;
    SIGINT_handler_struct_parent.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &SIGINT_handler_struct_parent, NULL); /*Calling Sigaction function */

    /*Check if arguments are passed correctly*/
    // Check usage & args
    if (argc < 2)
    {
        /*Print error on stdedd*/
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        /*return failure*/
        exit(1);
    }

    // Set up the address struct for this process (the server)
    // Clear out the address struct
    memset((char *)&serverAddress, '\0', sizeof(serverAddress));
    // Get the port number, convert to an integer from a string
    portNumber = atoi(argv[1]);
    // Create a network-capable socket
    serverAddress.sin_family = AF_INET;
    // Store the port number
    serverAddress.sin_port = htons(portNumber);
    // Any address is allowed for connection to this process
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Set up the socket
    listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
    if (listenSocketFD < 0)
        error("ERROR opening socket");

    // Enable the socket to begin listening
    if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
        error("ERROR on binding");

    // Start listening socket
    listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

    /*We listen once but accept multiple times*/
    while (1)
    {
        // Control fork count
        /*otp_d supports up to (and at most) five concurrent socket connections running at the same time*/
        if ((connections > 0) && (connections <= 5))
        {
            sizeOfClientInfo = sizeof(clientAddress);
            // Get the size of the address for the client that will connect
            establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
            /*Fork if a client is trying to connect*/
            pid = fork();
            /*Reduce connection pool*/
            connections = connections - 1;
            /*Bifuraction for parent and child*/

            if (pid == 0)
            {
                // Accept a connection, blocking if one is not available until one connects
                if (establishedConnectionFD < 0)
                    error("ERROR on accept");
                /* Sleep for 2 seconds */
                sleep(2);
                /*Step 1: Mode - get/post*/
                /*This child process of otp_d must first check if otp (see otp, below) is connecting in post or get mode*/
                memset(buffer, '\0', 512);
                charsRead = recv(establishedConnectionFD, buffer, 255, 0); // Read the client's message from the socket
                if (charsRead < 0)
                    error("ERROR reading from socket");
                /*Resetting message*/
                memset(msg, '\0', sizeof(msg));
                strcpy(msg, "OK");
                charsWritten = send(establishedConnectionFD, msg, strlen(msg), 0);
                /*Update mode variable*/
                char mode[512];
                memset(mode, '\0', 512);
                strcpy(mode, buffer);
                /*Update user name*/
                char user[512];
                memset(user, '\0', 512);
                /*Step 2: Username*/
                memset(buffer, '\0', 512);
                charsRead = recv(establishedConnectionFD, buffer, 255, 0); // Read the client's message from the socket
                strcpy(user, buffer);
                /*Handshake*/
                memset(msg, '\0', sizeof(msg));
                strcpy(msg, "OK");
                charsWritten = send(establishedConnectionFD, msg, strlen(msg), 0);

                if (strcmp(mode, "post") == 0)
                {
                    /*If otp has connected in post mode, then this child receives from otp a username and 
                    encrypted message via the communication socket (not the original listen socket). 
                    The otp_d child will then write the encrypted message to a file and print the path 
                    to this file (from the directory otp_d was started in) to stdout.*/

                    /*Check for file size errors*/
                    memset(msg, '\0', sizeof(msg));
                    recv(establishedConnectionFD, msg, strlen(msg), 0);
                    if (strcmp(msg, "OK"))
                    {
                        send(establishedConnectionFD, msg, sizeof(msg), 0);
                        /*Step 3: File Size of Encrypted Message*/
                        recv(establishedConnectionFD, buffer, sizeof(buffer), 0);
                        int file_size;
                        /*Update file size of encrypted message*/
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
                        /*Saving user encrypted file in the form " encrypted_[username]_[pid]*/
                        sprintf(encrypted_file_name, "encrypted_%s_%d", user, dollar_expansion);
                        fflush(stdout);
                        fprintf(stdout, "./%s\n", encrypted_file_name);
                        // Receive encrypted file
                        received_file = fopen(encrypted_file_name, "w");
                        if (received_file == NULL)
                        {
                            /*Return failure if file is corrupted*/
                            exit(EXIT_FAILURE);
                        }
                        /*Store the encrypted message on server storage*/
                        remain_data = file_size - 1;
                        int z = 0;
                        memset(buffer, '\0', file_size);
                        while (remain_data > 0)
                        {
                            charsRead = recv(establishedConnectionFD, buffer, sizeof(buffer), 0);
                            /*Check if bytes were transferred*/
                            if (charsRead > 0)
                            {
                                /*Placeholder variable to check sync of client and server*/
                                z = z + 1;
                                /*Write to file*/
                                fwrite(buffer, sizeof(char), charsRead, received_file);
                                /*Update remaining data*/
                                remain_data -= charsRead;
                            }
                        }
                        fclose(received_file);
                        /*Handshake*/
                        memset(msg, '\0', sizeof(msg));
                        strcpy(msg, "OK");
                        charsWritten = send(establishedConnectionFD, msg, strlen(msg), 0);
                        fflush(stdout);
                    }
                    /*Return the fork*/
                    return 0;
                }
                else if (strcmp(mode, "get") == 0)
                {
                    /*If otp has connected in get mode, then the child process 
                    of otp_d will receive from otp only a username. The child 
                    will retrieve the contents of the oldest file for this user
                     and send them to otp. Finally, it should delete the ciphertext file.*/

                    /* Step 5: Find the oldest encrypted file for the user*/
                    int oldestDirTime = INT_MAX;
                    char targetDirPrefix[2048];
                    /*Update target variable to search for relevant files*/
                    sprintf(targetDirPrefix, "encrypted_%s_", user);
                    static char oldestDirName[512];
                    /*Reset memory*/
                    memset(oldestDirName, '\0', sizeof(oldestDirName));
                    DIR *dirToCheck;
                    struct dirent *fileInDir;
                    struct stat dirAttributes;
                    /*Open current folder*/
                    dirToCheck = opendir(".");
                    /*Check if file descriptor is correctly updated*/
                    if (dirToCheck > 0)
                    {
                        /*Loop through all files in the . file*/
                        while ((fileInDir = readdir(dirToCheck)) != NULL)
                        {
                            /*Check for user encrypted files*/
                            if (strstr(fileInDir->d_name, targetDirPrefix) != NULL)
                            {
                                /*Check stats for user files*/
                                stat(fileInDir->d_name, &dirAttributes);
                                /*Identify the oldest file*/
                                if ((int)dirAttributes.st_mtime < oldestDirTime)
                                {
                                    oldestDirTime = (int)dirAttributes.st_mtime;
                                    /*Update variable*/
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
                    /*Handshake for checking if there are files present for the user*/
                    if (fp_message == NULL)
                    {
                        strcpy(msg, "No");
                    }
                    /*Send information to client*/
                    send(establishedConnectionFD, msg, sizeof(msg), 0);
                    recv(establishedConnectionFD, msg, sizeof(msg), 0);
                    /*if file is present for user*/
                    if (strcmp(msg, "OK") == 0)
                    {
                        fgets(ciphertext, sizeof(ciphertext), fp_message);
                        fclose(fp_message);
                        // /* Step 6: Know the filesize of encrypted file*/
                        struct stat file_stat;
                        int fd;
                        char file_size[512];
                        ssize_t len;
                        /*Open the last file*/
                        fd = open(oldestDirName, O_RDONLY);
                        if (fd == -1)
                        {
                            fprintf(stderr, "Error opening file --> %s", strerror(errno));
                            /*Return error if there is issue in opening file - identifying RACE condition*/
                            exit(EXIT_FAILURE);
                        }
                        /* Get file stats */
                        if (fstat(fd, &file_stat) < 0)
                        {
                            /*Check if there is issue in reading attributes of the file*/
                            fprintf(stderr, "Error fstat --> %s", strerror(errno));
                            exit(EXIT_FAILURE);
                        }
                        /*Update the variable*/
                        sprintf(file_size, "%ld", file_stat.st_size);
                        /* Step 8 : Send file size to client*/
                        len = send(establishedConnectionFD, file_size, sizeof(file_size), 0);
                        if (len < 0)
                        {
                            /*Redundant step to check is client is still responing and accepting data*/
                            fprintf(stderr, "Error on sending greetings --> %s", strerror(errno));
                            /*Return failure*/
                            exit(EXIT_FAILURE);
                        }
                        /*Handshake step*/
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
                            /*Check if bytes were successfully transferred*/
                            if (sent_bytes > 0)
                            {
                                remain_data -= sent_bytes;
                            }
                        }
                        /*Handshake from client: all data received*/
                        memset(msg, '\0', sizeof(msg));
                        charsRead = recv(establishedConnectionFD, msg, sizeof(buffer), 0);
                        /*Remove encryption message from server disk*/
                        remove(oldestDirName);
                        fflush(stdout);
                    }
                    // Close the existing socket which is connected to the client
                    close(establishedConnectionFD);
                    /*Returning the fork*/
                    return 0;
                }
            }
            else
            {
                ; // Not waiting for child
                // This will be handled by signal handler
            }
        }
    }
}
