
/*****************************************************************************
*  Author: Deirdre Moran
*  Program: otp_enc_d.c
*  Date: 3/17/2018
*  Description: Server-end decryption.  Sends decoded message back to client.
******************************************************************************/
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
// Maximum number of incoming connections
#define MAX_CONNECTIONS 10


/*********************************************************************************
*  Function: 		getSize()
*  Description:  	Gets size of file
*  Parameters:   	socket descriptor
*  Pre-conditions:	socket is connected
*  Post-conditions:	None
*  Return:		size of file
*********************************************************************************/
int getSize(int socketFD2)
{
				printf("IN GET SIZE");

	char fSize[20];
	memset(fSize, '\0', sizeof(fSize));
	// Get the file size
	if(recv(socketFD2, fSize, sizeof(fSize), 0) < 0){
		fprintf(stderr, "ERROR: Error receiving file size\n");
		exit(1);
	}
	// Convert file size to integer
	int size = atoi(fSize);
	return size;
}

/*********************************************************************************
*  Function: 		verifyID()
*  Description:  	Verifies ID of client (must be otp_enc)
*  Parameters:   	socket descriptor
*  Pre-conditions:	socket is connected
*  Post-conditions:	None
*  Return:		None
*********************************************************************************/
void verifyID(int socketFD2){
	char id[] = "OTP_DEC";
	char idBuffer[20];
	// Receive id from otp_enc, verify that this is correct client
	memset(idBuffer, '\0', sizeof(idBuffer));
	if(recv(socketFD2, idBuffer, sizeof(idBuffer), 0) < 0){
		fprintf(stderr, "ERROR: Error receving id from otp_dec");
		exit(1);
	}
	if(strncmp(idBuffer, id, 6) != 0){
		fprintf(stderr, "ERROR: otp_enc cannot use otp_dec_d");
		exit(1);
	}
}

/*********************************************************************************
*  Function: 		decode()
*  Description:  	Decodes ciphertext file with key
*  Parameters:   	ciphertext and key buffer, size of message, socket descriptor
*  Pre-conditions:	socket is connected, buffers and size defined
*  Post-conditions:	None
*  Return:		None
*********************************************************************************/
void decode(char * buffer, char * buffer2, int size, int socketFD2)
{
    	char decodedBuffer[MAX_BUFFER];
	memset(decodedBuffer, '\0', sizeof(decodedBuffer));
	// For each character in file data-buffer
	int i = 0;
	for(i = 0; i < size; i++){
		// Turn space into at symbol for both file and key buffers
		if (buffer[i] == ' '){
			buffer[i] = '@';
		}
		if (buffer2[i] == ' '){
			buffer2[i] = '@';
		}
		// Change chars to integers
	    	int ciphertext = (int) buffer[i];
		int myKey = (int) buffer2[i];
	        // Subtract 64 for ASCII conversion
	    	ciphertext = ciphertext - 64;
	    	myKey = myKey - 64;
	    	int modCrypt = ciphertext - myKey;
	    	if(modCrypt < 0){
			modCrypt = modCrypt + 27;
		}
		modCrypt = modCrypt + 64;
	        // Change integers to chars again
	    	decodedBuffer[i] = (char) modCrypt + 0;
	        // Turn at symbols to spaces again
	    	if (decodedBuffer[i] == '@'){
	       		decodedBuffer[i] = ' ';
	    	}
	}
	
	// Send the decoded message back to the client
	int k = size;
	// void pointer to decoded buffer
	void *ePtr = decodedBuffer;
	// while there are still bytes to be read
	while(k > 0){
		int bytes_sent = send(socketFD2, ePtr, k, 0);
		if(bytes_sent <= 0){
			printf("error writing bytes");
		}
		k -= bytes_sent;
		ePtr += bytes_sent;
	}
}

/*************************************************************************************
**************************************************************************************
**************************************************************************************
**************************************************************************************
*  Function: 		MAIN
*  Description:  	Sends decoded message back to client
*  Arguments:   	otp_enc_d [portnumber] &
**************************************************************************************
**************************************************************************************
**************************************************************************************
*************************************************************************************/
int main(int argc, char *argv[])
{
    	// variables for sockets, port number, pid, and buffers
    	int sockFD, sockFD2;
	int sPortNum;
	pid_t pid;
	// change the port number argument to integer
    	sPortNum = atoi(argv[1]);
	// Buffers for ciphertext and key files
	char buffer[MAX_BUFFER];
	char buffer2[MAX_BUFFER];

   	// Set up the socket
   	sockFD = socket(AF_INET, SOCK_STREAM, 0);
   	if(sockFD < 0){
        	fprintf(stderr, "ERROR: Socket error");
        	exit(1);
    	}
        // socket address struct
    	struct sockaddr_in sock;
    	sock.sin_family = AF_INET;
    	sock.sin_port = htons(sPortNum);
    	sock.sin_addr.s_addr = INADDR_ANY;

    	// bind the socket to the client address
    	if(bind(sockFD, (struct sockaddr *) &sock, sizeof(sock)) < 0){
        	fprintf(stderr, "ERROR: Socket binding");
        	exit(1);
    	}

    	// listen for up to 10 connections
    	if(listen(sockFD, MAX_CONNECTIONS) < 0){
    	    fprintf(stderr, "ERROR: Socket listening");
    	}

    	// Loop for server
    	while(1) {
		// Close socket if open
		close(sockFD2);
		// accept any incoming connections
        	sockFD2 = accept(sockFD, NULL, NULL);
        	if(sockFD2 < 0) {
            		fprintf(stderr, "ERROR: Socket Accept\n");
       			exit(1);
       		}

        	// fork a new process
        	pid = fork();
        	// If there was an error forking
        	if (pid < 0) {
        		fprintf(stderr, "ERROR: Forking\n");
       			exit(1);
       		}
		// If it is the child process
        	if (pid == 0){
			// verify ID
			verifyID(sockFD2);
			// get sizes of files
			int fileSize = getSize(sockFD2);
			int kSize = getSize(sockFD2);

			memset(buffer, '\0', sizeof(buffer));
			memset(buffer2, '\0', sizeof(buffer2));

			// Receive ciphertext data into buffer
			int fRecvd = fileSize;
			while(fRecvd > 0){
				int k = recv(sockFD2, buffer, sizeof(buffer), 0);
				if(k < 0){
					fprintf(stderr, "ERROR: reading");
					exit(1);
				}
				fRecvd -= k;
			}

			// receive key data into buffer2
			int kRecvd = fileSize;
			while(kRecvd > 0){
				int k = recv(sockFD2, buffer2, sizeof(buffer2), 0);
				if(k < 0){
					fprintf(stderr, "ERROR: reading");
					exit(1);
				}
				kRecvd -= k;
			}
			// decode message and send to client
			decode(buffer, buffer2, fileSize, sockFD2);
		}
		
		// If parent process, close socket
		if(pid > 0){
			close(sockFD2);
		}	// end of fork
	}  // end of server loop
	
	// close server socket
	close(sockFD);
}
