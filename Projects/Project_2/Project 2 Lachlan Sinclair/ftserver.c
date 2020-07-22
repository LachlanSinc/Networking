/********************************
* Project 2: Simple File Transfer
* Description: Implement a simple file transfer transfer system. This program acts
* as the server in the system.
* CS 372: Introduction to Computer Networks
* Last Modified: 12/1/2019
* Author: Lachlan Sinclair 
*
* Extra credit: My server is multithreaded
* ******************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <netdb.h>
#include <math.h>
#include <error.h>


#define ERROR(x) error_at_line(-1, -1, __FILE__, __LINE__, x)

/***********************************
 * References:
 * resource used to learn how to get the directory: https://www.geeksforgeeks.org/c-program-list-files-sub-directories-directory/
 * https://github.com/angrave/SystemProgramming/wiki/Networking,-Part-5:-Shutting-down-ports,-reusing-ports-and-other-tricks - this helped me
 * firgure out how to get the clients information to reuse in the other connection
 * https://stackoverflow.com/questions/3463426/in-c-how-should-i-read-a-text-file-and-print-all-strings - used this to refresh reading from files in C
 * https://stackoverflow.com/questions/8257714/how-to-convert-an-int-to-string-in-c/23840699 - proper way of converting a int to a string 
 * https://www.tutorialspoint.com/c_standard_library/c_function_fread.htm
 * project 1 in cs 372
 * My cs 344 project 4, encryption and decryption client programs
 * **********************************/


/**************************************
 * Function: commandCheck
 * Pre-conditions: strlen was run on the string and passed in as length, command in a valid string
 * Post-conditions: returns either 0, 1, or 2 indicating if the passed command is valid
 * Description: Returns 1 for a -l command, 2 for a -g command, or 0 for invalid command, where the command is a passed in string.
 * ***********************************/
int commandCheck(int length, char command[])
{

	//check if it is a valid -l command
	if(length==2 && command[0]=='-' && command[1]=='l')
		return 1;
	
	//check if it is a valid -g command
	else if(length>3 && command[0]=='-' && command[1]=='g' && command[2]==' ') 
		return 2;
	
	//else it is not a valid command return 0
	else
		return 0;
}

/**************************************
 * Function: addString
 * Pre-conditions: two strings, with both of there lengths are passed in. the passing function insures there is enough room in the 
 * string being copied into.
 * Post-conditions: The second string in concatinated onto the first string, with a carriage return attached after it
 * Description: This program adds one string to the end of another string, and attaches a carriage return at the end
 * ***********************************/
void addString(char *main, char *addition, int index, int length)
{
	int i=0;
	for(i;i<length;i++)
	{
		//copy the string into the end of the main string
		*(main+index+i)=*(addition+i);
	}
	//add a carriage return at the end of the string
	*(main+index+length)='\n';
}

/**************************************
 * Function: createDirString 
 * Pre-conditions: the datasocket has been set up
 * Post-conditions: the contents of the current directory have been sent to the client
 * Description: The program sends the contents of the current working directory to the client.
 * ***********************************/
void createDirString(int dataSocket)
{

	//intialize the string used to store the dir  to 1000 characters
	char *str = (char *)malloc(sizeof(char)*1024);
	char endOfDir[] = "%$#EndOfDir#$%";
	char inputBuffer[256];
	int charsRead, index =0; //index used to track the end of the string
	struct dirent *curDir;  

 	memset(str, '\0', 1024);

	DIR *lPoint = opendir("."); 
		  
	//handle any errors with opening the dir
 	if (lPoint == NULL) 
	{ 
		strcpy(str, "Could not open current directory\n"); 
	} 

	//loop through all elements in the dir
	while ((curDir = readdir(lPoint)) != NULL)
	{
		int length = strlen(curDir->d_name);	
		//check if the string will over flow	
		if((length+1+index)<1024)
		{
			//add the current name to the string and increment the index
			addString(str, curDir->d_name, index, length);	
			index+=length+1;
		}
		//overflow of the string will occur
		else
		{
			//send the current string to the client
			charsRead = send(dataSocket, str, strlen(str)-1, 0); //-1 to get rid of the last space
			charsRead = recv(dataSocket, inputBuffer, 5, 0);
			
			//reset the string and index
			memset(str, '\0', 1024);
			index=0;

			//add the current name to the string
			addString(str, curDir->d_name, index, length);	
			index+=length+1;
		}
	}

	//send the remaining string the client
	if(strlen(str)>0)
	{
		charsRead = send(dataSocket, str, strlen(str)-1, 0); //-1 to get rid of the last space
		memset(inputBuffer, '\0',256); //clear this for debugging purposes
		charsRead = recv(dataSocket, inputBuffer, 5, 0);//finish the handshake
	}

	//clean up the memory
	free(str);
	closedir(lPoint); 

}

/**************************************
 * Function: sendDirectory 
 * Pre-conditions: dataSocket has been set up correctly
 * Post-conditions: the directory has been sent, and post dir handshake is completed
 * Description: This calls the function the actually sends the dir, and then this completes a handshake after the dir is sent
 * and returns a bool int to the calling function based off of its success.
 * ***********************************/
int sendDirectory(int dataSocket)
{
	int charsRead;
	char dirGood[] = "dir is good";
	char endOfDir[] = "%$#EndOfDir#$%";
	char inputBuffer[256];

	//send the directory to the client
	createDirString(dataSocket);

	memset(inputBuffer, '\0', 256);

	//complete the post send handshake
	charsRead = send(dataSocket, endOfDir, strlen(endOfDir), 0);
	charsRead = recv(dataSocket, inputBuffer, strlen(dirGood), 0); 
	
	//make sure the client sucessfully got the dir, return bool based off of the compare
	if(strcmp(dirGood, inputBuffer)==0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/**************************************
 * Function: connectToData 
 * Pre-conditions: client is listening on the port number, the hostname is correct
 * Post-conditions: the datasocket is set up
 * Description: This function sets up the datasocket connection on the port passed by the client
 * ***********************************/
int connectToData(int portNumber, char hostName[])
{
	int socketFD;
	int result;
	int enableReuse = 1;
	int count = 100;

	//loop 100 times to allow for time to keep retrying to reuses or connect to the port	
	do {
		struct sockaddr_in serverAddress;
		struct hostent* serverHostInfo;
												
		memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
		serverAddress.sin_family = AF_INET; // Create a network-capable socket
		serverAddress.sin_port = htons(portNumber); // Store the port number
		serverHostInfo = gethostbyname(hostName); // Convert the machine name into a special form of address
		if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
		memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address
		socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
		if (socketFD < 0) ERROR("CLIENT: ERROR opening socket");
	
		//set  up the socket, allow reuse of sockets	
		result = setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(enableReuse));
		if (result < 0) 
		{
			printf("setsockopt result: %d\n", result);
			ERROR("CLIENT: ERROR set socket options");
		}
		
		//connect to the socket
		result = connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
		if (result == 0) 
		{
			return socketFD;//return the valid socket
		}
		count--;
	} while(count > 0);
	
        //if the code reaches here the conenction failed	
	printf("connect result: %d\n", result);
	// Connect socket to address
	ERROR("CLIENT: ERROR connecting");

}

/**************************************
 * Function: createServerSocket 
 * Pre-conditions: no process is currently running on portnumber on the local host
 * Post-conditions: the serversocket is setup on port number
 * Description: This method setups the server socket on portNumber
 * ***********************************/
int createServerSocket(int portNumber, struct sockaddr_in serverAddress)
{
	int listenSocketFD;
	socklen_t sizeOfClientInfo;
	
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) ERROR("ERROR opening socket");

	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		ERROR("ERROR on binding");

	return listenSocketFD; //return the socket
}

/**************************************
 * Function: checkForFile
 * Pre-conditions: fileName is a valid string
 * Post-conditions: returns a bool representing a files existence
 * Description: This function checks to see if the file with the name filename is in the current directory
 * ***********************************/
int checkForFile(char * fileName)
{

	FILE *fp = fopen(fileName, "r");
	if(fp == NULL)
		return 0; //cannot open the file
	//file was opened, close it a return true
	else
	{
		fclose(fp);
		return 1;	
	}
}

/**************************************
 * Function: streamFile 
 * Pre-conditions: filename is the name of a file that exists, datasocket is set up
 * Post-conditions: the client has been sent the file
 * Description: This function sends all of the data in a file to the client over the datasocket
 * ***********************************/
void streamFile(char * fileName, int dataSocket)
{
	int fileSize, sizeBuffer, charsRead, count, charSent, charsIn, eof=1;
	char buffer[1024];
	char done[]="%$#EOF#$%";
	FILE *fp = fopen(fileName, "r");
	fseek(fp, 0, SEEK_END); //go to then end of the file

	fileSize = ftell(fp); //get the file size
	sizeBuffer = (int)((ceil(log10(fileSize))+1)*sizeof(char));//determine how big the string we need to malloc is (see references)
	
	//create the string used to send the client the file size
	char *sizeString = (char *)malloc(sizeof(char)*sizeBuffer);
	memset(sizeString, '\0', sizeBuffer);
	sprintf(sizeString, "%d", fileSize);

	//send the client the size of the file
	charsRead = send(dataSocket, sizeString, strlen(sizeString), 0);
	memset(buffer, '\0', 1024);
	charsRead = recv(dataSocket, buffer, 1024, 0);//finish the handshake

	//make sure the clieant recieved the file
	if(strcmp(buffer, sizeString)!=0)
		return; //ERROR

	memset(buffer, '\0', 1024);
	
	//set fp back to the start of the file
	rewind(fp);

	//loop through the file sending chunks of 1024 bytes
	while(!feof(fp))
	{
		//read in 1024 bytes
		charsIn = (int)fread(buffer, 1, 1024, fp);
		fileSize-=charsIn;

		//check if everything has been read
		if(strlen(buffer)==0)
			break;
		//send the bytes read
		charsRead = send(dataSocket, buffer, 1024, 0);
		memset(buffer, '\0', 1024);
		
		//finish the handshake
		charsRead = recv(dataSocket, buffer, 1024, 0);
		memset(buffer, '\0', 1024);
	}
	fclose(fp);

}

int main(int argc, char *argv[])
{
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead, dataSocket, fileExists;
	socklen_t sizeOfClientInfo;
	char buffer[280];
	char clientName[256], clientPort[256]; //buffer used to hold the clients name
	char fileEx[]="#FileExists#";//some strings used as handshakes
	char fileInvalid[]="FILE NOT FOUND";
	char invalid[]="Invlaid command recieved.";
	struct sockaddr_in serverAddress, clientAddress;



	if (argc != 2){ fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args
	
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	
	//set up the server socket
	listenSocketFD = createServerSocket(portNumber, serverAddress);

	//listen for connections, queue up to four connections
	listen(listenSocketFD, 4);

	printf("Server open on %d.\n", portNumber);

	//infinite loop
	while(1)
	{
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) ERROR("ERROR on accept");
	
		//fork off process
		pid_t pid = fork();
		//handle errors during forking
		if(pid==-1)
		{
			//error("error creating child process.\n");
		};
		//children processes
		if(pid==0)
		{
			char validCommand[]="validCommand";
			int portSize, controlPort, dataPort, validCmd;
			char portBuffer[6], commandBuffer[256];
		
			memset(portBuffer, '\0', 6);
			memset(commandBuffer, '\0',256);
	
			//get infomration from the control socket to be used in setting up the data socket and print statements	
			getnameinfo((struct sockaddr *) &clientAddress, sizeOfClientInfo, clientName, sizeof(clientName), clientPort, 
				sizeof(clientPort),NI_NUMERICHOST | NI_NUMERICSERV);

			printf("Connection from %s.\n", clientName);

			//listen for the portnumber sent by a client
			charsRead = recv(establishedConnectionFD, portBuffer, 6, 0);
			dataPort = atoi(portBuffer);//convert the port number to an int

			//send back the port number and wait for the command to be sent
			charsRead = send(establishedConnectionFD, portBuffer, charsRead, 0);
			charsRead = recv(establishedConnectionFD, commandBuffer, 255, 0);

			//check to see if a valid command was sent
			validCmd = commandCheck(strlen(commandBuffer), commandBuffer);
		
			//recieved a valid -l command
			if(validCmd ==1)
			{
				printf("List directory requested on port %d.\n", dataPort);
				
				//let the client know it sent a valid command and complete the handshake
				charsRead = send(establishedConnectionFD, validCommand, 12, 0);
				charsRead = recv(establishedConnectionFD, buffer, 255, 0);//this is here to accomadate how the file seneding works
				
				//start the datasocket
				dataSocket = connectToData(dataPort, clientName);
				printf("Sending directory contents to %s:%d.\n", clientName, dataPort);
				
				//send the directory
				sendDirectory(dataSocket);
				close(dataSocket); //close the socket
			}

			//else if a -g command was recieved
			else if(validCmd == 2)
			{
				//let the lcient know it sent a valid command and complete the handshake
				charsRead = send(establishedConnectionFD, validCommand, 12, 0);
				charsRead = recv(establishedConnectionFD, buffer, 255, 0);

				//check to see if the file exists
				fileExists = checkForFile(commandBuffer+3);

				//if the file exists
				if(fileExists==1)
				{
					printf("File \"%s\" requested on port %d.\n", commandBuffer+3,dataPort);
					charsRead = send(establishedConnectionFD, fileEx, strlen(fileEx), 0); //let the client know the file exists
					charsRead = recv(establishedConnectionFD, buffer, 255, 0);//finish the handshake
					dataSocket = connectToData(dataPort, clientName); //open the datasocket
					printf("Sending \"%s\" to %s:%d.\n", commandBuffer+3, clientName, dataPort);
					streamFile(commandBuffer+3, dataSocket);//send the file over the datasocket
					close(dataSocket);//close the data socket
				}
				else
				{
					//let the client know the file does not exist
					printf("File not found. Sending error message to %s:%d.\n", clientName, dataPort);
					charsRead = send(establishedConnectionFD, fileInvalid, strlen(fileInvalid), 0);                   
					charsRead = recv(establishedConnectionFD, buffer, 255, 0);
				}

			}
			//else no valid command was recieved
			else
			{

				printf("Invalid command recieved, closing connection.\n");
				//let the client know it sent a invalid command and finish the handshake
				charsRead = send(establishedConnectionFD, invalid, strlen(invalid), 0);                                         
				charsRead = recv(establishedConnectionFD, buffer, 255, 0);

			}
			shutdown(establishedConnectionFD, 2);
		}
		//parent process
		else
		{
			close(establishedConnectionFD);
			wait(NULL); //clean up processes
		}
	}

	//code should reach not here
	close(listenSocketFD);
	return 0;
}
















