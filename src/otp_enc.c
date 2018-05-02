/***********************************************************************
*  Author: Deirdre Moran
*  Program: otp_enc.c
*  Date: 3/17/2018
*  Description: One-time-tab client side, sends key and plaintext files
*				for server side encoding
************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>

// Maximum buffer size
#define MAX_BUFFER 80000

/***********************************************************************
*  Function: 		sendEnc()
*  Description:  	Checks buffer for bad characters.  If none
*					present, sends data to server
*  Parameters:   	file and socket descriptors
*  Pre-conditions:	file is open, socket is connected
*  Post-conditions:	file is closed
*  Return:			None
************************************************************************/
void sendEnc(int fileD[2], int sockFD){
	int i, j, count = 0;
	char dataBuffer[MAX_BUFFER];
	char goodChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

	// Clear the data buffer for retrieving contents of plaintext file
	memset(dataBuffer, '\0', sizeof(dataBuffer));

	// Read contents into buffer
	int k = read(fileD[1], dataBuffer, sizeof(dataBuffer));

	// if error reading, error message
	if(k < 0){
		fprintf(stderr, "ERROR: Reading file");
		exit(1);
	}

	// Check buffer for bad characters
	for(i = 0; i < k; i++){
		for(j = 0; j < 27; j++){
			if(dataBuffer[i] == goodChars[j]){
				count++;
				j = 26;
			}
		}
	}
	// If file contains bad characters, error and exit
	if(count != k - 1){
		fprintf(stderr, "ERROR: Bad characters");
		exit(1);
	}

	// void pointer to data buffer
	void *p = dataBuffer;
	// While there are still bytes to be read
	// Send file contents stored in databuffer to server
	while(k > 0){
		int bytes_written = send(sockFD, p, k, 0);
		if(bytes_written <= 0){
			fprintf(stderr, "ERROR: Error writing bytes");
			exit(1);
		}
		k -= bytes_written;
		p += bytes_written;
	}
}

/***********************************************************************
*  Function: 		recvEncMsg()
*  Description:  	Receives and prints encoded message from server
*  Parameters:   	socket descriptor and size of message
*  Pre-conditions:	socket is connected
*  Post-conditions:	None
*  Return:			None
************************************************************************/
void recvEncMsg(int sockFD, int fSize){
	int i;
	// buffer to hold encoded message from server
	char dataBuffer[MAX_BUFFER];
	// size of message
	int totalBytes = fSize;
	// clear the buffer
	memset(dataBuffer, '\0', sizeof(dataBuffer));

	// Receive encoded message and store in buffer
	while(totalBytes > 0){
		int k = recv(sockFD, dataBuffer, sizeof(dataBuffer), 0);
		if(k < 0){
			fprintf(stderr, "ERROR: Error receiving encoded message");
			exit(1);
		}
		totalBytes -= k;
	}

	// Print encoded message in buffer
	for(i = 0; i < fSize; i++){
		printf("%c", dataBuffer[i]);
	}
	printf("\n");

}

/***********************************************************************
*  Function: 		verifyID()
*  Description:  	Sends id (filename) to server, verifies self as
*					otp_enc (server should only accept otp_enc)
*  Parameters:   	socket descriptor
*  Pre-conditions:	socket is connected
*  Post-conditions:	None
*  Return:			None
************************************************************************/
void verifyID(int sockFD){
	char idStr[20];
	char id[] = "OTP_ENC";
	sprintf(idStr, "%s ", id);

	// Size of id string
	int k = sizeof(id);

	// void pointer to id string
	void *p = id;

	// While there are still bytes to be read
	// Send string contents stored in idStr to server
	while(k > 0){
		int bytes_written = send(sockFD, p, k, 0);
		if(bytes_written <= 0){
			fprintf(stderr, "ERROR: Error sending id bytes");
			exit(1);
		}
		k -= bytes_written;
		p += bytes_written;
	}
}

/***********************************************************************
*  Function: 		sendSizes()
*  Description:  	Sends size of file to encoding server
*  Parameters:   	file and socket descriptors
*  Pre-conditions:	socket is connected
*  Post-conditions:	None
*  Return:			size of file
************************************************************************/
int sendSizes(char * file, int sockFD){
	char sizeStr[20];

	// get size of file
	struct stat st;
	stat(file, &st);
	int mysize = st.st_size;
	mysize--;
	memset(sizeStr, '\0', sizeof(sizeStr));
	sprintf(sizeStr, "%d ", mysize);
	int k = sizeof(sizeStr);

	// void pointer to size string
	void *p = sizeStr;

	// While there are still bytes to be read
	// Send string contents stored in idStr to server
	while(k > 0){
		int bytes_written = send(sockFD, p, k, 0);
		if(bytes_written <= 0){
			fprintf(stderr, "ERROR: Error sending size bytes");
			exit(1);
		}
		k -= bytes_written;
		p += bytes_written;
	}
	return mysize;
 }




/*************************************************************************************
**************************************************************************************
**************************************************************************************
**************************************************************************************
*  Function: 		MAIN
*  Description:  	Sends plaintext and key files to encoding server,
*					receives encoded message
*  Arguments:   	otp_enc [plaintext filename] [key filename] [server portnumber]
*  Pre-conditions:	otp_enc_c is running on server portnumber
**************************************************************************************
**************************************************************************************
**************************************************************************************
*************************************************************************************/
int main(int argc, char *argv[])
{
	// socket file descriptor
	int socketFD;
	int portNumber;

	// File descriptors
	int fd[2];
	// integer version of plaintext file size
	int pTextSize;

	// Convert the port number to an integer
	portNumber = atoi(argv[3]);

	// Set up server address structure
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	memset((char*)&serverAddress, '\0', sizeof(serverAddress));
	// Create a network-capable socket
	serverAddress.sin_family = AF_INET;
	// Store the port number
	serverAddress.sin_port = htons(portNumber);
	// Convert the machine name into a special form of address
	serverHostInfo = gethostbyname("localhost");
	if (serverHostInfo == NULL) {
		fprintf(stderr, "ERROR, no such host\n");
		exit(1);
	}
	// Copy in the address
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);
	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0);
		if (socketFD < 0) {
			fprintf(stderr, "ERROR: Cannot open socket");
			exit(1);
	}

	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
		fprintf(stderr, "ERROR: Cannot connect to server");
	}

	// Send id to verify self as encoding client
	verifyID(socketFD);

	// open plaintext file specified in command line argument
	fd[1] = open(argv[1], O_RDONLY, 0664);
	// If error opening file
	if(fd[1] == -1){
		fflush(stdout);
		fprintf(stderr, "ERROR: Cannot open file");
		exit(1);
	}

	// Else file is open
	else{
		// Get the size of plaintext and key files, send to server
		pTextSize = sendSizes(argv[1], socketFD);
		int keyS = sendSizes(argv[2], socketFD);
		// If key is less than plaintext file, key is too short
		if(keyS < pTextSize){
			fprintf(stderr, "ERROR: key is too short\n");
			exit(1);
		}
		// Send the contents of file to encoding server
		sendEnc(fd, socketFD);
	}
	// Close the file
	close(fd[1]);

	// Open key file
	fd[1] = open(argv[2], O_RDONLY, 0664);
	// If error opening file
	if(fd[1] == -1){
		fflush(stdout);
		fprintf(stderr, "ERROR: Cannot open file");
		exit(1);
	}

	// Send the contents of file to encoding server
	else{
		// Send the contents of file to encoding server
		sendEnc(fd, socketFD);
	}
		// close the file
	close(fd[1]);

	// Receive and print encoded message
	recvEncMsg(socketFD, pTextSize);

	// Close the socket
	close(socketFD);

	return 0;
}
