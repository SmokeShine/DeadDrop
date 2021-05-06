/*
 ============================================================================
 Name        : otp.c
 Author      : Prateek Gupta
 Version     : v1
 ============================================================================
 */

/*Usage
otp post user plaintext key port
otp get user key port
*/

/* Test Script Results Sample
$ ./otp post Prateek plaintext1 key36 5555
Returns: ./encrypted_Prateek_74530 on server
$ ./otp get Prateek key36 5555
THE RED GOOSE FLIES AT MIDNIGHT STOP
$ ./otp get Prateek key36 5555 > myplaintext
$ ./otp get Prateek key36 5555 > myplaintext &
*/

/*Library Imports*/
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

/*Error handlers for socket connections*/
void error(const char *msg)
{
	// Error function used for reporting issues
	perror(msg);
	exit(0);
}

/*Convert a character to its index value*/
/*
A	1
B	2
C	3
D	4
E	5
F	6
G	7
H	8
I	9
J	10
K	11
L	12
M	13
N	14
O	15
P	16
Q	17
R	18
S	19
T	20
U	21
V	22
W	23
X	24
Y	25
Z	26
	0

*/
int convert_to_index(char s)
{
	/*Hack for dealing with spaces*/
	if (s == (char)32)
	{
		/*If character is space, return 0 for easier additions*/
		return 0;
	}
	else
	{
		/*Return index of alphabet*/
		return (int)s - 64;
	}
}

/*Converts index to char*/
/*
1	A
2	B
3	C
4	D
5	E
6	F
7	G
8	H
9	I
10	J
11	K
12	L
13	M
14	N
15	O
16	P
17	Q
18	R
19	S
20	T
21	U
22	V
23	W
24	X
25	Y
26	Z
0	
*/
char convert_to_char(int i)
{
	/*Hack for dealing with spaces*/
	if (i == 0)
	{
		return (char)32;
	}
	else
	{
		/*Return character*/
		return (char)i + 64;
	}
}

int main(int argc, char *argv[])
{
	/*Helper variables*/
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent *serverHostInfo;
	char buffer[75000];
	int port_index = -1;
	int sent_bytes = 0;
	int i;
	char file_size[512];
	char msg[512];
	struct stat file_stat;
	off_t offset;
	int remain_data;
	char FILE_TO_SEND_message[512];
	char FILE_TO_SEND_key[512];
	int fd;
	ssize_t len;
	// Check usage & args
	if (argc < 3)
	{
		/*Print to stderr if arguments are not provided*/
		fprintf(stderr, "USAGE: %s method user message port\n", argv[0]);
		/*Return failure*/
		exit(EXIT_FAILURE);
	}

	/*Initial check for invalid characters*/
	if (strcmp(argv[1], "post") == 0)
	{
		/*Get Port index*/
		port_index = 5;
		/*Get message file */
		strcpy(FILE_TO_SEND_message, argv[3]);

		// Check for invalid message
		FILE *fp_test = fopen(argv[3], "r");
		char test_lines[75000];
		if (fp_test == NULL)
		{
			/*If message file does not exist, print error to stderr*/
			fprintf(stderr, "C -Failed to open file foo --> %s\n", strerror(errno));
			/*Return failure*/
			exit(EXIT_FAILURE);
		}
		/*Read message file*/
		fgets(test_lines, sizeof(test_lines), fp_test);
		fclose(fp_test);
		/*Loop through the message data to check if only characters are provided*/
		for (i = 0; i <= strlen(test_lines) - 1; i++)
		{
			// Check if english characters are provided
			if (!(test_lines[i] >= (char)65 && test_lines[i] <= (char)122))
			{
				/*Exception for spaces and line breaks*/
				if (test_lines[i] != (char)32 && test_lines[i] != (char)10)
				{
					/*Print error on stderr*/
					fprintf(stderr, "Bad Characters in %s\n", argv[3]);
					/*Return failure*/
					exit(EXIT_FAILURE);
				}
			}
		}
	}
	else if (strcmp(argv[1], "get") == 0)
	{
		/*Get portnumber index for get mode*/
		port_index = 4;
	}

	// Set up the server address struct
	// Clear out the address struct
	memset((char *)&serverAddress, '\0', sizeof(serverAddress));
	// Get the port number, convert to an integer from a string
	portNumber = atoi(argv[port_index]);
	// Create a network-capable socket
	serverAddress.sin_family = AF_INET;
	// Store the port number
	serverAddress.sin_port = htons(portNumber);
	// Convert the machine name into a special form of address
	serverHostInfo = gethostbyname("localhost");

	/*Placeholder function for checking host if trying to use network*/
	if (serverHostInfo == NULL)
	{
		/*print error to stderr informing host is invalid*/
		fprintf(stderr, "CLIENT: ERROR, no such host\n");
		exit(0);
	}
	// Copy in the address
	memcpy((char *)&serverAddress.sin_addr.s_addr, (char *)serverHostInfo->h_addr, serverHostInfo->h_length);

	// Set up the socket
	// Create the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFD < 0)
		error("CLIENT: ERROR opening socket");

	// Connect to server
	if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");

	/*Step 1: Sending  mode to server - get/post*/
	memset(buffer, '\0', sizeof(buffer));
	strcpy(buffer, argv[1]);

	// Send message to server
	charsWritten = send(socketFD, buffer, strlen(buffer), 0);
	if (charsWritten < 0)
		error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(buffer))
		printf("CLIENT: WARNING: Not all data written to socket!\n");
	/*Confirmation from server*/
	// Clear out the buffer again for reuse
	memset(msg, '\0', sizeof(msg));
	charsRead = recv(socketFD, msg, 512, 0);
	/*Step 2: Sending user name to the server*/
	// Clear out the buffer again for reuse
	memset(buffer, '\0', sizeof(buffer));
	strcpy(buffer, argv[2]);

	charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
	if (charsWritten < 0)
		error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(buffer))
		printf("CLIENT: WARNING: Not all data written to socket!\n");

	// Clear out the buffer again for reuse
	memset(buffer, '\0', sizeof(buffer));

	memset(msg, '\0', sizeof(msg));
	charsRead = recv(socketFD, msg, 512, 0);

	if (strcmp(argv[1], "post") == 0)
	{
		/*Send 3: Send file size of message*/
		fd = open(FILE_TO_SEND_message, O_RDONLY);
		if (fd == -1)
		{
			/*Print to stderr; cannot open file*/
			fprintf(stderr, "Error opening file --> %s", strerror(errno));
			/*Return Failure*/
			exit(EXIT_FAILURE);
		}

		/* Get file stats */
		if (fstat(fd, &file_stat) < 0)
		{
			/*Error on fetching file details*/
			fprintf(stderr, "Error fstat --> %s", strerror(errno));
			/*Return failure*/
			exit(EXIT_FAILURE);
		}
		/*Save size to variable*/
		sprintf(file_size, "%ld", file_stat.st_size);

		/*Step 4.1: Read Key File*/
		char key_lines[sizeof(buffer)];

		FILE *fp = fopen(argv[4], "r");
		/*Read key data*/
		fgets(key_lines, sizeof(key_lines), fp);
		/*Check for file size*/
		if (strlen(key_lines) < file_stat.st_size)
		{
			/*If key size is smaller than message size, we cannot use the key*/
			fprintf(stderr, "Error: key ‘%s’ is too short -\n", argv[4]);
			/*Return failure*/
			exit(EXIT_FAILURE);
		}

		/* Sending file size */
		len = send(socketFD, file_size, sizeof(file_size), 0);
		/*Redundant check to check if network is stable*/
		if (len < 0)
		{
			fprintf(stderr, "Error on sending greetings --> %s", strerror(errno));
			/*Return Failure*/
			exit(EXIT_FAILURE);
		}
		/*Handshake confirmation*/
		memset(msg, '\0', sizeof(msg));
		charsRead = recv(socketFD, msg, 512, 0);
		/*Step 4.2: Read Message File*/
		char message_lines[sizeof(buffer)];

		FILE *fp_message = fopen(argv[3], "r");
		/*Read mesage file*/
		if (fp_message == NULL)
		{
			fprintf(stderr, "C -Failed to open file foo --> %s\n", strerror(errno));
			/*Return error*/
			exit(EXIT_FAILURE);
		}
		fgets(message_lines, sizeof(key_lines), fp_message);
		fclose(fp_message);

		/*Step 4.3: Encrypt Data*/
		/*Fixing Case*/
		/*Convert message to all caps: Redundant and can be removed for performance gain*/
		for (i = 0; i <= strlen(message_lines); i++)
		{
			message_lines[i] = toupper(message_lines[i]);
		}
		/*Convert message to all caps: Redundant and can be removed for performance gain*/
		for (i = 0; i <= strlen(key_lines); i++)
		{
			key_lines[i] = toupper(key_lines[i]);
		}
		/*Preparation for encryption*/
		/*
		“Suppose Alice wishes to send the message "HELLO" to Bob. Assume two pads of paper containing identical random sequences of letters were somehow previously produced and securely issued to both. Alice chooses the appropriate unused page from the pad. The way to do this is normally arranged for in advance, as for instance 'use the 12th sheet on 1 May', or 'use the next available sheet for the next message'.

		The material on the selected sheet is the key for this message. Each letter from the pad will be combined in a predetermined way with one letter of the message. (It is common, but not required, to assign each letter a numerical value, e.g., "A" is 0, "B" is 1, and so on.)

		In this example, the technique is to combine the key and the message using modular addition. The numerical values of corresponding message and key letters are added together, modulo 26. So, if key material begins with "XMCKL" and the message is "HELLO", then the coding would be done as follows:

			H       E       L       L       O  message
		7 (H)   4 (E)  11 (L)  11 (L)  14 (O) message
		+ 23 (X)  12 (M)   2 (C)  10 (K)  11 (L) key
		= 30      16      13      21      25     message + key
		=  4 (E)  16 (Q)  13 (N)  21 (V)  25 (Z) message + key (mod 26)
			E       Q       N       V       Z  → ciphertext
		If a number is larger than 26, then the remainder, after subtraction of 26, is taken [as the result]. This simply means that if the computations "go past" Z, the sequence starts again at A.

		*/
		char *ciphertext = (char *)malloc(strlen(message_lines) + 1);
		memset(ciphertext, '\0', strlen(message_lines));
		int temp = 0;
		int temp_message_key = 0;
		/*Loop through all characters and encrypt based on the helper functions*/
		for (i = 0; i < strlen(message_lines) - 1; i++) 
		{
			temp = convert_to_index(message_lines[i]) + convert_to_index(key_lines[i]);
			/*We have 27 characters including space*/
			temp_message_key = temp % 27;
			ciphertext[i] = convert_to_char(temp_message_key);
		}
		/*Adding new character for valid string*/
		ciphertext[strlen(message_lines) - 1] = '\n';
		/*Step 5: Send encrypted file to the server */
		off_t offset = 0;
		int remain_data = atoi(file_size) - 1;
		/* Sending file data */
		int sent_bytes = 0;
		/*Avoid multiple iterations on loop for performance*/
		while (remain_data > 0)
		{
			sent_bytes = send(socketFD, ciphertext, strlen(ciphertext), 0);
			/*Check if we were able to send the bytes*/
			if (sent_bytes > 0)
			{
				remain_data -= sent_bytes;
			}
		}
		/*Handshake*/
		memset(msg, '\0', sizeof(msg));
		charsRead = recv(socketFD, msg, sizeof(buffer), 0);
	}
	else if (strcmp(argv[1], "get") == 0)
	{
		/*Initial handshake*/
		memset(msg, '\0', sizeof(msg));
		recv(socketFD, msg, sizeof(msg), 0); 
		send(socketFD, msg, sizeof(msg), 0);
		/*Check if there are messages for user*/
		if (strcmp(msg, "No") == 0)
		{
			/*print to stderr about no new messages*/
			fprintf(stderr, "There are no messages for %s\n", argv[2]);
			/*Return failure*/
			exit(EXIT_FAILURE);
		}

		/* Step 3.1: Read the key file*/
		int i = 0;
		char key[sizeof(buffer)];
		FILE *fp = fopen(argv[3], "r");
		/*Save data to array*/
		fgets(key, sizeof(key), fp);
		fclose(fp);
		/*Step 3.2: Fixing Case*/
		/*Redundant step; could be removed for performance*/
		for (i = 0; i <= strlen(key); i++)
		{
			key[i] = toupper(key[i]);
		}
		/*Step 4: Receive encrypted file size*/
		memset(buffer, '\0', sizeof(buffer));
		recv(socketFD, buffer, sizeof(buffer), 0);
		int encrypted_file_size = atoi(buffer);
		char *ciphertext = (char *)malloc((encrypted_file_size + 1) * sizeof(char));
		/*Handshake*/
		memset(msg, '\0', sizeof(msg));
		strcpy(msg, "OK");
		charsWritten = send(socketFD, msg, strlen(msg), 0);
		/*Step 5: Receive encrypted file*/
		FILE *received_file;
		int remain_data = 0;
		ssize_t len;
		/*Dollar explansion: Getting pid for saving file*/
		unsigned int dollar_expansion;
		dollar_expansion = getpid();
		char encrypted_file_name[1024];
		/*Updating variable with username and pid*/
		sprintf(encrypted_file_name, "decrypted_%s_%d", argv[2], dollar_expansion);
		received_file = fopen(encrypted_file_name, "w");
		if (received_file == NULL)
		{
			/*If unable to retrieve file, update the user*/
			fprintf(stderr, "Failed to open file foo --> %s\n", strerror(errno));
			/*Return failure*/
			exit(EXIT_FAILURE);
		}
		/*Wait for server to send the encrypted file*/
		remain_data = encrypted_file_size;
		while (remain_data > 0)
		{
			memset(buffer, '\0', sizeof(buffer));
			charsRead = recv(socketFD, buffer, sizeof(buffer), 0);
			/*if received bytes, write to the file*/
			if (charsRead > 0)
			{
				fwrite(buffer, sizeof(char), charsRead, received_file);
				/*update remaining data*/
				remain_data -= charsRead;
			}
		}
		fclose(received_file);
		/*Read Encrypted data received from disk*/
		FILE *fp_encrypted = fopen(encrypted_file_name, "r");
		fgets(ciphertext, encrypted_file_size + 1, fp_encrypted); /*This is a sad function which will cause more issues in the future*/
		fclose(fp_encrypted);

		memset(msg, '\0', sizeof(msg));
		strcpy(msg, "OK");
		charsWritten = send(socketFD, msg, strlen(msg), 0);
		/* Step 7: Run Decryption*/
		/*Information on decryption*/
		/*
		The ciphertext to be sent to Bob is  "EQNVZ". Bob uses the matching key page and the same process, but in reverse, to obtain the plaintext. Here the key is subtracted from the ciphertext, again using modular arithmetic:

			E       Q       N       V       Z  ciphertext
			4 (E)  16 (Q)  13 (N)  21 (V)  25 (Z) ciphertext
		-  23 (X)  12 (M)   2 (C)  10 (K)  11 (L) key
		= -19       4      11      11      14     ciphertext – key
		=   7 (H)   4 (E)  11 (L)  11 (L)  14 (O) ciphertext – key (mod 26)
			H       E       L       L       O  → message
		Similar to the above, if a number is negative then 26 is added to make the number zero or higher.

		Thus Bob recovers Alice's plaintext, the message "HELLO". Both Alice and Bob destroy the key sheet immediately after use, thus preventing reuse and an attack against the cipher.”

		*/
		/*Decrypt*/
		int temp = 0;
		int temp_message_key = 0;
		/*Array to save decryption*/
		char *solution = (char *)malloc((encrypted_file_size) * sizeof(char));
		memset(solution, '\0', sizeof(solution));
		for (i = 0; i < encrypted_file_size; i++) 
		{
			/*Hack for space for encryted text*/
			if (ciphertext[i] == (char)32)
			{
				ciphertext[i] = (char)64;
			}
			/*Hack for space for key in hand*/
			if (key[i] == (char)32)
			{
				key[i] = (char)64;
			}
			/*Subtraction step*/
			temp = ciphertext[i] - key[i];
			/*logic to deal with negative remainders*/
			temp_message_key = ((temp % 27) + 27) % 27;
			/*Convert index back to character*/
			solution[i] = convert_to_char(temp_message_key);
		}
		/*updates for valid string*/
		solution[encrypted_file_size] = '\0';
		solution[encrypted_file_size - 1] = '\n';
		fflush(stdout);
		/*print on screen*/
		fprintf(stdout, "%s", solution);
		/*remove helper file*/
		remove(encrypted_file_name);
		/*return success*/
		return 0;
	}
	/*Close the socket*/
	close(socketFD);
	return 0;
}
