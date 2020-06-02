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

int main(int argc, char *argv[])
{

	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent *serverHostInfo;

	char buffer[75000];
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

	/*Step 1: Sending  mode to server - get/post*/
	memset(buffer, '\0', sizeof(buffer));
	strcpy(buffer, argv[1]);

	// Send message to server
	charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
	if (charsWritten < 0)
		error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(buffer))

		memset(msg, '\0', sizeof(msg));
	charsRead = recv(socketFD, msg, 512, 0);
	/*Step 2: Sending user name to the server*/
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	strcpy(buffer, argv[2]);

	charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
	if (charsWritten < 0)
		error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(buffer))
		//printf("CLIENT: WARNING: Not all data written to socket!\n");

		memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse

	memset(msg, '\0', sizeof(msg));
	charsRead = recv(socketFD, msg, 512, 0);

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

		sprintf(file_size, "%ld", file_stat.st_size);

		/*Step 4.1: Read Key File*/
		char key_lines[sizeof(buffer)];

		FILE *fp = fopen(argv[4], "r");

		fgets(key_lines, sizeof(key_lines), fp);
		if (strlen(key_lines) < file_stat.st_size)
		{
			fprintf(stderr, "Error: key ‘%s’ is too short -\n", argv[4]);
			exit(EXIT_FAILURE);
		}

		/* Sending file size */
		len = send(socketFD, file_size, sizeof(file_size), 0);
		if (len < 0)
		{
			fprintf(stderr, "Error on sending greetings --> %s", strerror(errno));

			exit(EXIT_FAILURE);
		}

		memset(msg, '\0', sizeof(msg));
		charsRead = recv(socketFD, msg, 512, 0);
		/*Step 4.2: Read Message File*/
		char message_lines[sizeof(buffer)];

		FILE *fp_message = fopen(argv[3], "r");

		if (fp_message == NULL)
		{
			fprintf(stderr, "C -Failed to open file foo --> %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		fgets(message_lines, sizeof(key_lines), fp_message);
		fclose(fp_message);

		/*Step 4.3: Encrypt Data*/
		/*Fixing Case*/
		int i;
		for (i = 0; i <= strlen(message_lines); i++)
		{
			message_lines[i] = toupper(message_lines[i]);
		}

		for (i = 0; i <= strlen(key_lines); i++)
		{
			key_lines[i] = toupper(key_lines[i]);
		}

		char *ciphertext = (char *)malloc(strlen(message_lines) + 1);
		memset(ciphertext, '\0', strlen(message_lines));
		int temp = 0;
		int temp_message_key = 0;
		for (i = 0; i < strlen(message_lines) - 1; i++) /*Excluding null at the end*/
		{
			temp = convert_to_index(message_lines[i]) + convert_to_index(key_lines[i]);

			temp_message_key = temp % 27;
			ciphertext[i] = convert_to_char(temp_message_key);
		}
		ciphertext[strlen(message_lines) - 1] = '\n';
		/*Step 5: Send encrypted file to the server */

		off_t offset = 0;
		int remain_data = atoi(file_size) - 1;
		/* Sending file data */
		int sent_bytes = 0;
		while (remain_data > 0)
		{
			sent_bytes = send(socketFD, ciphertext, strlen(ciphertext), 0);

			if (sent_bytes > 0)
			{
				remain_data -= sent_bytes;
			}
		}

		memset(msg, '\0', sizeof(msg));
		charsRead = recv(socketFD, msg, sizeof(buffer), 0);
	}
	else if (strcmp(argv[1], "get") == 0)
	{
		memset(msg, '\0', sizeof(msg));
		recv(socketFD, msg, sizeof(msg), 0); //check if message exist
		send(socketFD, msg, sizeof(msg), 0);
		if (strcmp(msg, "No") == 0)
		{
			fprintf(stderr, "There are no messages for %s\n", argv[2]);
			exit(EXIT_FAILURE);
		}

		/* Step 3.1: Read the key file*/

		int i = 0;
		char key[sizeof(buffer)];
		FILE *fp = fopen(argv[3], "r");

		fgets(key, sizeof(key), fp);

		fclose(fp);
		/*Step 3.2: Fixing Case*/
		for (i = 0; i <= strlen(key); i++)
		{
			key[i] = toupper(key[i]);
		}
		/*Step 4: Receive encrypted file size*/
		memset(buffer, '\0', sizeof(buffer));
		recv(socketFD, buffer, sizeof(buffer), 0);
		int encrypted_file_size = atoi(buffer);
		char *ciphertext = (char *)malloc((encrypted_file_size + 1) * sizeof(char));

		memset(msg, '\0', sizeof(msg));
		strcpy(msg, "OK");
		charsWritten = send(socketFD, msg, strlen(msg), 0);

		/*Step 5: Receive encrypted file*/
		FILE *received_file;
		int remain_data = 0;
		ssize_t len;
		received_file = fopen("solution", "w");
		if (received_file == NULL)
		{
			fprintf(stderr, "Failed to open file foo --> %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		remain_data = encrypted_file_size;
		while (remain_data > 0)
		{
			memset(buffer, '\0', sizeof(buffer));
			charsRead = recv(socketFD, buffer, sizeof(buffer), 0);

			if (charsRead > 0)
			{
				fwrite(buffer, sizeof(char), charsRead, received_file);
				remain_data -= charsRead;
			}
		}
		fclose(received_file);

		/*Read Encrypted data received from disk*/
		FILE *fp_encrypted = fopen("solution", "r");

		fgets(ciphertext, encrypted_file_size + 1, fp_encrypted); /*This is a sad function which will cause more issues in the future*/

		fclose(fp_encrypted);

		memset(msg, '\0', sizeof(msg));
		strcpy(msg, "OK");
		charsWritten = send(socketFD, msg, strlen(msg), 0);
		/* Step 7: Run Decryption*/
		/*Decrypt*/
		int temp = 0;
		int temp_message_key = 0;
		char *solution = (char *)malloc((encrypted_file_size + 1) * sizeof(char));
		memset(solution, '\0', sizeof(solution));

		for (i = 0; i < encrypted_file_size; i++) /*Excluding null at the end*/
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
			solution[i] = convert_to_char(temp_message_key);
		}

		solution[encrypted_file_size - 1] = '\n';
		// solution[encrypted_file_size + 1] = '\0';
		fprintf(stdout, "%s", solution);
		remove("solution");
		return 0;
	}
	close(socketFD);
	return 0;
}
