/***********************************************************************
*  Author: Deirdre Moran
*  Program: otp_enc_d.c
*  Date: 3/17/2018
*  Description: Program creates a key file of specified length.
*				syntax:   keygen keyLength
************************************************************************/
//Program creates a key file of specified length.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
int main(int argc, char *argv[])
{
	if(argc < 2){
		fprintf(stderr, "Arguments must be of form: 'keygen <keylength>'\n");
		exit(1);
	}
	int i = 0;
	int j = 0;
	int randoms[27];
	char myArray[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	char * ptr = myArray;
	int lengthA = atoi(argv[1]);
	char A[lengthA];
	int * ptrR = randoms;
	ptrR[0] = 'm';
	int checker = 0;
	srand(time(0));
	for(i = 0; i < lengthA; i++){
		int r = rand() % 27;
		 A[i] = ptr[r];
	}
	A[i] = '\n';
	for(j= 0; j < sizeof(A) + 1; j++)
	{
		printf("%c", A[j]);
	}
	return 0;
}

