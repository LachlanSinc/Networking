/********************************
 * Program 1: Chatclient
 * Description: Chat program using a client server connection, via a
 * TCP protocol connection
 * CS 372: Introduction to Computer Networks
 * Last Modified: 11/3/2019 
 * ******************************/

//Citations: My cs 344 project 4, encryption and decryption client programs
//These were in turn cited the example code given in block 4 of CS 344

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX 11 //constant used to determine the max size of the userhandle two extra chars for "> "

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

/**************************************
 * Function: getUserHandle 
 * Pre-conditions: userHandle has already been declared.
 * Post-conditions: User has input the user handle
 * Description: Prompts the user to enter their handle, must be 10 characters or less.
 * The userhandle will be saved and used throughout the program.
 * ***********************************/
int getUserHandle(char userHandle[], int userHandleLength)
{
	//clear the userhandle buffer
	memset(userHandle, '\0', userHandleLength);
	printf("Enter your user handle(Max 10 characters): ");  //prompt the user for input
	fflush(stdout);
	fgets(userHandle, MAX, stdin);				//read in the user input
	int newLine = strcspn(userHandle, "\n");

	//check if less than the max chars were used
	if (newLine+1 < MAX) 
	{
	  userHandle[newLine] = '\0'; //append the null
	} 
	else 
	{
     	  while ((getchar()) != '\n'); //clear stdin
	}

	int length = strlen(userHandle); //use the strlen to append the "> "
	userHandle[length] = '>';
	userHandle[length+1] =  ' ';
	
	return strlen(userHandle);				//return the users handlelength

}

/**************************************
 * Function: initiateContact
 * Pre-conditions: userHandle has been recieved from user, the user input a valid port
 * number while executing the program  
 * Post-conditions: TCP connection is established with chatserve, server handle is saved
 * Description: The client makes first contact with the server by sending it the userhandle, and
 * receives the serverHandle. Server handle and user handle will no longer be sent for effeciency.
 * see READ ME file.
 * ***********************************/
int initiateContact(char * passedPortNumber, char * hostName, char * userHandle, char * serverHandle, int userHandleLength, int serverHandleLength)
{
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char firstContactBuffer[513];		//used to recieve the serverhandle
							
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(passedPortNumber); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname(hostName); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address
														
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");
																
																
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
			error("CLIENT: ERROR connecting");

	charsWritten = send(socketFD, userHandle, userHandleLength, 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < userHandleLength) printf("CLIENT: WARNING: Not all data written to socket!\n");

																					
	memset(serverHandle, '\0', serverHandleLength); //clear the server handle char array
	memset(firstContactBuffer, '\0', sizeof(firstContactBuffer)); //clear the serverhandle buffer

	charsRead = recv(socketFD, firstContactBuffer, sizeof(firstContactBuffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");

	strcpy(serverHandle, firstContactBuffer); //copy the server handle into the serverhandle char array

	return socketFD; //return the socket
}

/**************************************
 * Function: sendMessage
 * Pre-conditions: initiate contact was executed properly
 * Post-conditions: message is sent to the chatserve. Updates the close connection bool.
 * Description: prompts the user for input to send to the chatserve using the socket.
 * Checks to see if the user entered \quit.
 * ***********************************/
int sendMessage (int socketFD, char buffer[], int bufferSize, char * quitString, char * userHandle)
{
	int charsSent; //tracks the number of chars sent

	printf(userHandle); //display the users handle
	fflush(stdout);
	memset(buffer, '\0', bufferSize); // Clear out the buffer array
	fgets(buffer, bufferSize-1, stdin); // Get input from the user, trunc to buffer - 1 chars, leaving \0
	
	if(buffer[0] ==  '\n') //check for the user just pressing enter
		return 2;

	buffer[strcspn(buffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds


	charsSent = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
	if (charsSent < 0) error("CLIENT: ERROR writing to socket");
	if (charsSent < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	//return true if the user entered \quit
	if(strcmp(buffer, quitString)==0)
		return 1;



	//return false when the user doesnt enter \quit
	return 0;
}

/**************************************
 * Function: recieveMessage 
 * Pre-conditions: a message has been sent by sendMessage
 * Post-conditions: a message has been read from the chatserve and the close connection bool has been updated
 * Description: Listens for a message on the socket, checks to see if it is \quit, if it is not it displays the
 * message.
 * ***********************************/
int recieveMessage(int socketFD, char buffer[], int bufferSize, char * quitString, char * serverHandle)
{
	int charsRecieved; //track the number of chars read

	memset(buffer, '\0', bufferSize); // Clear out the buffer again for reuse
	charsRecieved = recv(socketFD, buffer, bufferSize- 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRecieved < 0) error("CLIENT: ERROR reading from socket");
		
	if(strcmp(buffer, quitString)==0) //return true if the message was \quit, dont display the message
		return 1;

	printf("%s%s\n", serverHandle,buffer); //display the message from the server along with its handle

	return 0; //return false when the message is not \quit
}

int main(int argc, char *argv[])
{
	int socketFD, charsWritten, charsRead, handleLength ,bufferSize,
	    userHandleLength, serverHandleLength;
	int closeConnection;		//bool used to track when to close a connection
	char buffer[501]; 		//buffer used for send messages back and forth
	char quitString[7]="\\quit"; 	//string used to check for quiting
	char userHandle[13]; 		//stores the user handle
	char serverHandle[13]; 		//stores the server handle
					    
	if (argc != 3) { fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); exit(0); } // Check usage & args

	bufferSize=sizeof(buffer); //save the size of buffer to use in function calls
	userHandleLength = sizeof(userHandle); //used for function calls
	serverHandleLength = sizeof(serverHandle); //used for function calls
	
	//get the userhandle and handle length by using the getUserHandle function
	handleLength = getUserHandle(userHandle, userHandleLength);

	//call the intiate function to pass user handles back and forth, and set up the socket
	socketFD = initiateContact(argv[2], argv[1], userHandle, serverHandle, userHandleLength, serverHandleLength);

	//loop until \quit is recieved
	while(1)
	{	

		//call the sent message
		closeConnection = sendMessage(socketFD, buffer, bufferSize,  quitString, userHandle);
		
		//handle the user trying to send nothing
		while(closeConnection == 2)
		{
			printf("Please enter a message or \\quit to quit.\n");
			closeConnection = sendMessage(socketFD, buffer, bufferSize,  quitString, userHandle);
		}

		//break out of the loop if the client entered \quit
		if(closeConnection == 1)
			break;

		//wait for a message from the chatserve
		closeConnection = recieveMessage(socketFD, buffer, bufferSize, quitString, serverHandle);

		//break out of the loop if the serve sent \quit
		if(closeConnection == 1)
			break;
	}

	close(socketFD); // Close the socket
	return 0;
}










