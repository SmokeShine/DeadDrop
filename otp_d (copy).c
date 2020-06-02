
/* Library Imports */
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <ftw.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/sendfile.h>

int convert_to_index(char s)
{
    if (s == (char)32)
    {
        return 0;
    }
    else
    {
        return (int)s - 64;
    }
}

char convert_to_char(int i)
{
    if (i == 0)
    {
        return (char)32;
    }
    else
    {
        return (char)i + 64;
    }
}

void error(const char *msg)
{
    perror(msg);
    exit(1);
} // Error function used for reporting issues

int main(int argc, char *argv[])
{
    int connections = 0;
    int charsWritten = -1;
    pid_t pid = -5;
    char msg[512];
    int status = -5;
    int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
    socklen_t sizeOfClientInfo;
    char buffer[75000];
    struct sockaddr_in serverAddress, clientAddress;

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

    listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections
    while (1)
    {
        // Accept a connection, blocking if one is not available until one connects
        sizeOfClientInfo = sizeof(clientAddress);
        // Get the size of the address for the client that will connect
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
        if (establishedConnectionFD < 0)
            error("ERROR on accept");

        pid = fork();
        if (pid == 0)
        {

            /* Sleep for 2 seconds */
            sleep(2);
            /*Step 1: Mode - get/post*/
            memset(buffer, '\0', 512);
            charsRead = recv(establishedConnectionFD, buffer, 255, 0); // Read the client's message from the socket
            if (charsRead < 0)
                error("ERROR reading from socket");
            printf("SERVER: I received this from the client: \"%s\"\n", buffer);
            /*Send confirmation*/

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
            printf("%s - %s\n", user, mode);
            printf("\n\n\n\n\n\n-----------%s----------\n\n\n\n\n", user);
            memset(msg, '\0', sizeof(msg));
            strcpy(msg, "OK");
            charsWritten = send(establishedConnectionFD, msg, strlen(msg), 0);

            if (strcmp(mode, "post") == 0)
            {
                /*Step 3: File Size of Message*/
                recv(establishedConnectionFD, buffer, sizeof(buffer), 0);
                int file_size;
                file_size = atoi(buffer);
                printf("\n\n\n\n\n\n-----------%s----------\n\n\n\n\n", user);
                memset(msg, '\0', sizeof(msg));
                strcpy(msg, "OK");
                charsWritten = send(establishedConnectionFD, msg, strlen(msg), 0);
                printf("\n\n\n\n\n\n-----------%s----------\n\n\n\n\n", user);
                /* Step 4: Receive Message*/
                FILE *received_file;
                int remain_data = 0;
                ssize_t len;
                received_file = fopen(user, "w");
                if (received_file == NULL)
                {
                    fprintf(stderr, "D - Failed to open file foo --> %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }

                remain_data = file_size;
                int z = 0;
                printf("Remaining Data is %d\n", remain_data);
                memset(buffer, '\0', file_size);
                printf("\n\n\n\n\n\n-----------%s----------\n\n\n\n\n", user);
                while (remain_data > 0)
                {

                    charsRead = recv(establishedConnectionFD, buffer, sizeof(buffer), 0);

                    if (charsRead > 0)
                    {
                        z = z + 1;
                        fwrite(buffer, sizeof(char), charsRead, received_file);
                        remain_data -= charsRead;
                        fprintf(stdout, "Receive %d bytes \n", charsRead);
                    }
                    printf("Remaining Data is %d\n", remain_data);
                }
                printf("-----------------%d ------------------------------------------------\n", z);
                printf("\n\n\n\n\n\n-----------%s----------\n\n\n\n\n", user);

                printf("Bytes Pending %d \n", remain_data);
                fclose(received_file);

                memset(msg, '\0', sizeof(msg));
                strcpy(msg, "OK");
                charsWritten = send(establishedConnectionFD, msg, strlen(msg), 0);

                /*Step 5: Key Filename*/
                char key_filename[512];
                memset(buffer, '\0', 512);
                charsRead = recv(establishedConnectionFD, buffer, sizeof(buffer), 0); // Read the client's message from the socket
                strcpy(key_filename, buffer);
                printf("%s - %s - %s\n", user, mode, key_filename);

                memset(msg, '\0', sizeof(msg));
                strcpy(msg, "OK");
                charsWritten = send(establishedConnectionFD, msg, strlen(msg), 0);

                /* Step 6: Read the key file*/
                int i = 0;
                char key_lines[sizeof(buffer)];
                FILE *fp = fopen(key_filename, "r");

                fgets(key_lines, sizeof(key_lines), fp);
                // printf("Key Loaded for %s -%s", user, key_lines);

                fclose(fp);
                /* Step 7: Read the message file*/
                /**/
                char message_lines[sizeof(buffer)];

                FILE *fp_message = fopen(user, "r");

                printf("\n\n\n\n\n\n-----------%s----------\n\n\n\n\n", user);
                if (fp_message == NULL)
                {
                    fprintf(stderr, "C -Failed to open file foo --> %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                fgets(message_lines, sizeof(key_lines), fp_message);
                printf("Message Loaded for %s -%s\n", user, message_lines);

                fclose(fp_message);
                /*Step 8: Delete the temp file*/
                int del = remove(user);
                if (!del)
                    printf("The temp file is Deleted successfully\n");
                else
                    printf("the temp file is not Deleted\n");
                /*Step 9: Fixing Case*/
                for (i = 0; i <= strlen(message_lines); i++)
                {
                    message_lines[i] = toupper(message_lines[i]);
                }

                for (i = 0; i <= strlen(key_lines); i++)
                {
                    key_lines[i] = toupper(key_lines[i]);
                }

                printf("Fixed Formats\n");
                printf("Message: %s\n", message_lines);
                printf("Key: %s\n", key_lines);
                /*Step 10: Add both of them - ascii value?*/

                char *ciphertext = (char *)malloc(strlen(message_lines));
                memset(ciphertext, '\0', strlen(message_lines));
                int temp = 0;
                int temp_message_key = 0;
                for (i = 0; i < strlen(message_lines) - 1; i++) /*Excluding null at the end*/
                {
                    temp = convert_to_index(message_lines[i]) + convert_to_index(key_lines[i]);

                    temp_message_key = temp % 27;
                    printf("%d - %d", convert_to_index(message_lines[i]), convert_to_index(key_lines[i]));
                    ciphertext[i] = convert_to_char(temp_message_key);
                }
                printf("Ciphertext is %s and its length is %ld\n", ciphertext, strlen(ciphertext));
                /*Step 11: Save to Disk*/
                unsigned int dollar_expansion;
                dollar_expansion = getpid();
                char encrypted_file_name[1024];
                sprintf(encrypted_file_name, "encrypted_%s_%d", user, dollar_expansion);
                FILE *fp_cipher_message = fopen(encrypted_file_name, "w");
                if (fp_cipher_message == NULL)
                {
                    fprintf(stderr, "B -Failed to open file foo --> %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                fwrite(ciphertext, strlen(ciphertext), 1, fp_cipher_message);
                printf("Cipher saved for %s -%s\n", user, ciphertext);

                fclose(fp_cipher_message);
                printf("Encryption Complete for %s\n", user);
                printf("Encrypted File Name is %s\n", encrypted_file_name);
            }
            else if (strcmp(mode, "get") == 0)
            {
                /*Step 3: Key Filename*/
                char key_filename[512];
                memset(buffer, '\0', 512);
                charsRead = recv(establishedConnectionFD, buffer, sizeof(buffer), 0); // Read the client's message from the socket
                strcpy(key_filename, buffer);
                printf("%s - %s - %s\n", user, mode, key_filename);

                memset(msg, '\0', sizeof(msg));
                strcpy(msg, "OK");
                charsWritten = send(establishedConnectionFD, msg, strlen(msg), 0);

                /* Step 4: Read the key file*/
                int i = 0;
                char key[sizeof(buffer)];
                FILE *fp = fopen(key_filename, "r");

                fgets(key, sizeof(key), fp);
                printf("Key Loaded for %s -%s", user, key);

                fclose(fp);
                /*Step 4.1: Fixing Case*/
                for (i = 0; i <= strlen(key); i++)
                {
                    key[i] = toupper(key[i]);
                }
                printf("Upper %s\n", key);
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
                            printf("%s\n", fileInDir->d_name);
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
                printf("\t %s\n", oldestDirName);
                closedir(dirToCheck);

                /* Step 5.01: Read the encryption file*/

                char ciphertext[sizeof(buffer)];

                FILE *fp_message = fopen(oldestDirName, "r");
                if (fp_message == NULL)
                {
                    fprintf(stderr, "A - Failed to open file foo --> %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                fgets(ciphertext, sizeof(key), fp_message);
                printf("Message Loaded for %s -%s\n", user, ciphertext);

                fclose(fp_message);

                /* Step 6: Know the filesize of encrypted file*/
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

                fprintf(stdout, "File Size: \n%ld bytes\n", file_stat.st_size);
                sprintf(file_size, "%ld", file_stat.st_size);
                /* Step 7: Run Decryption*/
                /*Decrypt*/
                int temp = 0;
                int temp_message_key = 0;
                char *solution = (char *)malloc((atoi(file_size) + 1) * sizeof(char));
                memset(solution, '\0', sizeof(solution));
                for (i = 0; i < atoi(file_size); i++) /*Excluding null at the end*/
                {

                    if (ciphertext[i] == (char)32)
                    {
                        ciphertext[i] = (char)64;
                    }
                    if (key[i] == (char)32)
                    {
                        key[i] = (char)64;
                    }

                    temp = ciphertext[i] - key[i];

                    temp_message_key = ((temp % 27) + 27) % 27;
                    // printf("%d\t%s\n",strlen(solution),convert_to_char(temp_message_key));
                    solution[i] = convert_to_char(temp_message_key);
                    // printf("temp \t cipher  \t key \t temp_message_key \n");
                    // printf("%d \t %d  \t\t %d \t %d \n", temp, ciphertext[i], key[i], temp_message_key);
                }
                solution[atoi(file_size)] = '\n';
                solution[atoi(file_size) + 1] = '\0';
                printf("\n%s%ld\n", solution, strlen(solution));
                sprintf(file_size, "%ld", strlen(solution));

                /* Step 8 : Send file size to client*/
                len = send(establishedConnectionFD, file_size, sizeof(file_size), 0);
                if (len < 0)
                {
                    fprintf(stderr, "Error on sending greetings --> %s", strerror(errno));

                    exit(EXIT_FAILURE);
                }

                memset(msg, '\0', sizeof(msg));
                charsRead = recv(establishedConnectionFD, msg, sizeof(buffer), 0);
                printf("----%s\n", msg);

                /* Step 9 : Send the decrypted file to client*/

                off_t offset = 0;
                int remain_data = strlen(solution);
                printf("Remaining Data %d\n", remain_data);

                /* Sending file data */
                int sent_bytes = 0;
                while (remain_data > 0)
                {
                    sent_bytes = send(establishedConnectionFD, solution, strlen(solution), 0);

                    if (sent_bytes > 0)
                    {
                        printf("Bytes Transferred %d\n", sent_bytes);
                        remain_data -= sent_bytes;
                    }
                    printf("Remaining Data %d\n", remain_data);
                }

                memset(msg, '\0', sizeof(msg));
                charsRead = recv(establishedConnectionFD, msg, sizeof(buffer), 0);
                printf("%s\n", msg);

                // memset(msg, '\0', sizeof(msg));
                // strcpy(msg, "OK");
                // charsWritten = send(establishedConnectionFD, msg, strlen(msg), 0);

                /* Step 10 : Delete keygen file*/
                int del = remove(key_filename);
                if (!del)
                    printf("The temp file is Deleted successfully\n");
                else
                    printf("the temp file is not Deleted\n");

                /* Step 11: Delete encrypted file*/
                remove(oldestDirName);
            }
            printf("-------\n");

            /*Closing for Child*/
            close(establishedConnectionFD); // Close the existing socket which is connected to the client
        }
        else
        {
            waitpid(pid, &status, 0);
            printf("Parent\n");
            // close(listenSocketFD); // Close the listening socket
        }
    }
}
