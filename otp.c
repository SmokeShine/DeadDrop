#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <ctype.h>

// #define FILE_TO_SEND_message "plaintext1"

void error(const char *msg)
{
	perror(msg);
	exit(0);
} // Error function used for reporting issues

int convert_to_index(char s)
{
	if (s == (char)32)
	{
		return 27;
	}
	else
	{
		return (int)s - 64;
	}
}

char convert_to_char(int i)
{
	if (i == 27)
	{
		return (char)32;
	}
	else
	{
		return (char)i + 64;
	}
}

int main(int argc, char *argv[])
{

	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent *serverHostInfo;
	
	char buffer[512];
	int port_index = -1;
	int sent_bytes = 0;
	
	char file_size[512];
	char msg[512];
	struct stat file_stat;
	off_t offset;
	int remain_data;
	char FILE_TO_SEND_message[512];
	char FILE_TO_SEND_key[512];
	int fd;
	ssize_t len;
	
	if (argc < 3)
	{
		fprintf(stderr, "USAGE: %s method user message port\n", argv[0]);
		exit(EXIT_FAILURE);
	} // Check usage & args
	if (strcmp(argv[1], "post") == 0)
	{
		port_index = 5;
		strcpy(FILE_TO_SEND_message, argv[3]);
	}
	else if (strcmp(argv[1], "get") == 0)
	{
		port_index = 4;
	}
	
	// Set up the server address struct
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	// printf("%s\n",argv[port_index]);
	portNumber = atoi(argv[port_index]);						 // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET;							 // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber);					 // Store the port number
	serverHostInfo = gethostbyname("localhost");				 // Convert the machine name into a special form of address
	
	if (serverHostInfo == NULL)
	{
		fprintf(stderr, "CLIENT: ERROR, no such host\n");
		exit(0);
	}
	memcpy((char *)&serverAddress.sin_addr.s_addr, (char *)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address
	
	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0)
		error("CLIENT: ERROR opening socket");

	// Connect to server
	if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");

	// Get input message from user
	/*	printf("CLIENT: Enter text to send to the server, and then hit enter: ");
	memset(buffer, '\0', sizeof(buffer));	  // Clear out the buffer array
	fgets(buffer, sizeof(buffer) - 1, stdin); // Get input from the user, trunc to buffer - 1 chars, leaving \0
	buffer[strcspn(buffer, "\n")] = '\0';	  // Remove the trailing \n that fgets adds
	*/

	/*Step 1: Sending  mode to server - get/post*/
	memset(buffer, '\0', sizeof(buffer));
	strcpy(buffer, argv[1]);
	
	// Send message to server
	charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
	if (charsWritten < 0)
		error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(buffer))
		printf("CLIENT: WARNING: Not all data written to socket!\n");
	
	memset(msg, '\0', sizeof(msg));
	charsRead = recv(socketFD, msg, 512, 0);
	printf("%s\n", msg);
	
	/*Step 2: Sending user name to the server*/
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	strcpy(buffer, argv[2]);
	
	charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
	if (charsWritten < 0)
		error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(buffer))
		printf("CLIENT: WARNING: Not all data written to socket!\n");

	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	
	memset(msg, '\0', sizeof(msg));
	charsRead = recv(socketFD, msg, 512, 0);
	printf("%s\n", msg);

	if (strcmp(argv[1], "post") == 0)
	{
		
		/*Send 3: Send file size of message*/
		fd = open(FILE_TO_SEND_message, O_RDONLY);
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
		/* Sending file size */
		len = send(socketFD, file_size, sizeof(file_size), 0);
		if (len < 0)
		{
			fprintf(stderr, "Error on sending greetings --> %s", strerror(errno));

			exit(EXIT_FAILURE);
		}

		memset(msg, '\0', sizeof(msg));
		charsRead = recv(socketFD, msg, 512, 0);
		printf("%s\n", msg);

		/*Step 4: Send file to the server */
		offset = 0;
		remain_data = file_stat.st_size;
		printf("Remaining Data %d\n", remain_data);
		/* Sending file data */
		while (remain_data > 0)
		{
			sent_bytes = sendfile(socketFD, fd, &offset, 255);

			if (sent_bytes > 0)
			{
				printf("Bytes Transferred %d\n", sent_bytes);
				remain_data -= sent_bytes;
			}
			printf("Remaining Data %d\n", remain_data);
		}

		memset(msg, '\0', sizeof(msg));
		charsRead = recv(socketFD, msg, 512, 0);
		printf("%s\n", msg);

		/*Step 5: Send key filename*/
		memset(buffer, '\0', sizeof(buffer));
		strcpy(buffer, argv[4]);
		charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
		if (charsWritten < 0)
			error("CLIENT: ERROR writing to socket");
		if (charsWritten < strlen(buffer))
			printf("CLIENT: WARNING: Not all data written to socket!\n");
		memset(msg, '\0', sizeof(msg));
		charsRead = recv(socketFD, msg, 512, 0);
		printf("%s\n", msg);

		/* Step 6: Read the key file - why is this required?*/
		int i = 0;
		char lines[1000][512];
		FILE *fp = fopen(argv[4], "r");
		while (i < 1000 && fgets(lines[i], sizeof(lines[0]), fp))
		{
			lines[i][strlen(lines[i]) - 1] = '\0';
			i = i + 1;
		}
		printf("Key Loaded for %s\n", argv[2]);
		fclose(fp);
	}

	/**/
	if (strcmp(argv[1], "get") == 0)
	{
		/*Step 3: Send key filename*/
		memset(buffer, '\0', sizeof(buffer));
		strcpy(buffer, argv[3]);
		charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
		if (charsWritten < 0)
			error("CLIENT: ERROR writing to socket");
		if (charsWritten < strlen(buffer))
			printf("CLIENT: WARNING: Not all data written to socket!\n");
		memset(msg, '\0', sizeof(msg));
		charsRead = recv(socketFD, msg, 512, 0);
		printf("%s\n", msg);
		/*Step 4: Receive decrypter file size*/
		memset(buffer, '\0', sizeof(buffer));
		recv(socketFD, buffer, 512, 0);
		printf("Decrypted File Size is %s\n", buffer);
		int decrypted_file_size = atoi(buffer);

		memset(msg, '\0', sizeof(msg));
		strcpy(msg, "OK");
		charsWritten = send(socketFD, msg, strlen(msg), 0);

		/*Step 5: Receive decrypted file*/
		FILE *received_file;
		int remain_data = 0;
		ssize_t len;
		received_file = fopen("solution", "w");
		if (received_file == NULL)
		{
			fprintf(stderr, "Failed to open file foo --> %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		remain_data = decrypted_file_size;
		printf("Remaining Data is %d\n", remain_data);
		while (remain_data > 0)
		{
			memset(buffer, '\0', 512);
			charsRead = recv(socketFD, buffer, 512, 0);

			if (charsRead > 0)
			{
				fwrite(buffer, sizeof(char), charsRead, received_file);
				remain_data -= charsRead;
				fprintf(stdout, "Receive %d bytes \n", charsRead);
			}
			printf("Remaining Data is %d\n", remain_data);
		}
		printf("Bytes Pending %d \n", remain_data);
		fclose(received_file);

		memset(msg, '\0', sizeof(msg));
		strcpy(msg, "OK");
		charsWritten = send(socketFD, msg, strlen(msg), 0);


		/*Step 6: Remove keygen file*/
		remove(argv[3]);


		/*Step 7: For stdout*/
		int i = 0;
		char output[512];
		FILE *fp = fopen("solution", "r");

		fgets(output, sizeof(output), fp);

		fclose(fp);

		/*Step 8: Remove decrypted file*/
		remove("solution");
		fflush(stdout);
		printf("%s",output);
	}
	close(socketFD);
	return 0;
}
