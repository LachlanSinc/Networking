###################################
# Project 2: Simple File Tansfer
# Description: Implement a simple file transfer system. This program acts as the clisnt in the system
# CS372: Introduction to Computer Networks
# LastModified: 12/1/2019
# Author: Lachlan Sinclair
###################################

import os.path
import sys
from socket import *

# References:
# https://stackoverflow.com/questions/37372603/how-to-remove-specific-substrings-from-a-set-of-strings-in-python
# https://linuxize.com/post/python-check-if-file-exists/
# https://www.w3schools.com/python/python_file_write.asp
# https://stackoverflow.com/questions/5040491/python-socket-doesnt-close-connection-properly
# https://www.w3schools.com/python/ref_string_split.asp
# https://docs.python.org/release/2.6.5/library/internet.html
# CS372 lecture 15, python code for the client side


###################################
# Function:establishConnection
# Pre-conditions: sername is a valid server name, the server is listening on serverport, dataport is unused
# Post-conditions: the control socket is set up
# Description: takes the user input and connects to the server by sending the dataport it would like to use
###################################
def establishConnection(serverName, serverPort, dataPort):
	
	#set up and connect to the socket
	clientSocket = socket(AF_INET, SOCK_STREAM)
	clientSocket.connect((serverName,serverPort))

	#send and receive the dataport
	clientSocket.send(str(dataPort))
	returnedPort = clientSocket.recv(1024)

	#convert the return port to a int
	intPort = int(returnedPort)
	
	#make sure the server recieved the correct port
	if dataPort != intPort:
		print("Data port was corrupted.")	

	#return the socket
	return clientSocket


###################################
# Function: sendCommand
# Pre-conditions: client socket is setupt, and a command was give while running the program
# Post-conditions: the client has sent the command and recieved a response
# Description: The client send the command to the server and waits for a reponse, the server 
# responds by letting the client know if a valid command was sent.
###################################
def sendCommand(clientSocket, command):

	#send the command to the server, then complete a handshake
	clientSocket.send(command)
	valString = clientSocket.recv(1024)
	clientSocket.send("recieved")

	#check if the server says it recieved a valid command
	if valString == "validCommand":
		#print("valid command sent")
		return 1
			
	else:
		#print out the error message
		print valString
		return 0
	
	
###################################
# Function: dataPort
# Pre-conditions: the user entered a data port that isnt in use
# Post-conditions: the datasocket is set up
# Description: setups the data connection socket
###################################
def startDataConnection(dataPort):
	
	serverSocket = socket(AF_INET, SOCK_STREAM)#define the socket
        serverSocket.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)#allow for reuse
	serverSocket.bind(('', dataPort))
	serverSocket.listen(1)
	connectionSocket, addr = serverSocket.accept()
	return connectionSocket #return the socket


###################################
# Function: getDirectory
# Pre-conditions: serverSocket has been setup
# Post-conditions: the directory of the server has been printed out
# Description: This function prints out the directory sent by the server
###################################
def getDirectory(serverSocket):

	#recieve and print out the first part of the directory
	directory = serverSocket.recv(1024)
	serverSocket.send("part")
	print directory

	#loop until the rest of the directory has been printed out
	while 1:
		directory = serverSocket.recv(1024)
		if "%$#EndOfDir#$%" in directory: #if its the end of the directory break out
			break
		serverSocket.send("part")#finish the handshake
		print directory #print the remaining parts of the directory

	#let the server know the entire directory has been recieved
	serverSocket.send("dir is good")	


###################################
# Function: createFileName
# Pre-conditions: filename and counter have been defined as a string and a int
# Post-conditions: a new name based off of the file name and counter has been created
# Description: Creates a new string based off of filename and counter, this is used to make new file names
###################################
def createFileName(fileName, counter):
	temp = fileName.split(".txt") #split to get just the file name
	fileName = temp[0]+"_"+str(counter)+"_"+".txt" #attach _number_ to the end of the filename and readd .txt
	return fileName


###################################
# Function: checkForFile
# Pre-conditions: filename is a valid string
# Post-conditions: returns either filename or a new unique file name
# Description: This function is used to insure files dont overwrite one another, checks if a name is unique
# if it isnt it creates a new unique one
###################################
#make sure this works with files names that are all numbers
def checkForFile(fileName):
	#check if the current name already exists in this dir
	if os.path.isfile(fileName):
		counter = 1
		#try new names until a unique once is found
		while os.path.isfile(createFileName(fileName, counter)):
			counter += 1
		#return the unique name
		return createFileName(fileName, counter)	
	else:
		return fileName	#return the passed in name since it is unique

###################################
# Function: getFile
# Pre-conditions: serversocket is set up and filename is a valid file name
# Post-conditions: the file is copied from the server
# Description: This function copies a file over the datasocket
###################################
def getFile(serverSocket, fileName):

	goodStr = "gooodStr"

	#get the file size from the server, send it back for confirmation
	fileSize = serverSocket.recv(1024)
	serverSocket.send(fileSize)

	#cast the size to a int 
	count = int(fileSize)
	f = open(fileName, "a") #append to the file

	#while all of the bytes havent been read
	bytesRead = 0;
	while count > 0:

		#read in 1024 bytes
		if (count - 1024) > 0:
			f.write(serverSocket.recv(1024))
			count -= 1024
		#read in the remaining bytes
		else:
			f.write(serverSocket.recv(count))
			count = 0
		#let the server know all bytes have been read
		serverSocket.send(goodStr)

	f.close() #close the file

###################################
# Function: checkFile
# Pre-conditions: clientsocket is setup, the user provided a valid severname and dataport
# Post-conditions: return true of false if the server has the file, will display a message if it doesnt
# Description: This method checks to see if the file requested exists on the server
###################################
def checkFile(clientSocket, serverName, dataPort):
	exists = "#FileExists#"

	#read in the server response, send the rest of the handshake
	recieved = clientSocket.recv(1024)
	clientSocket.send("recieved")

	#if it exists return true, if it doesnt print the message received and return false
	if exists in recieved:
		return 1
	else:
		print serverName+":"+str(dataPort)+" says "+recieved
		return 0



def main():

	#check input
	if len(sys.argv) != 5 and len(sys.argv) != 6:
		print "USAGE: python ftclient servername serverport commands dataport"
		exit(1)
	#read in the data
	serverName = sys.argv[1]
	serverPort = int(sys.argv[2])
	command = sys.argv[3]
	
	#read in the rest of the data using the format for -l
	if len(sys.argv) == 5:
		dataPort = int(sys.argv[4])
	
	#read in the res of the data using the format for -g
	elif len(sys.argv) == 6:
		fileName = sys.argv[4]
		dataPort = int(sys.argv[5])
	#the incorrect amount of arguements provided
	else:
		print("Error: incorrect input.\n")
		exit()

	#set up the client connection
	clientSocket = establishConnection(serverName, serverPort, dataPort)

	#if 6 commands were sent it is a file request
	if len(sys.argv) == 6:
		#create the full command
		fullCommand = command+" "+fileName
		#check if the command is valid
		valid = sendCommand(clientSocket, fullCommand)
		
		#if its a valid command
		if valid == 1:
			#check if the file exists
			fileExists = checkFile(clientSocket, serverName, dataPort)
			if fileExists == 1:
				#start of the data connection
				serverSocket = startDataConnection(dataPort)
				#get a unique file name
				validName = checkForFile(fileName)
				print "Recieving "+fileName+" from "+serverName+":"+str(dataPort)
				#get the file from the server
				getFile(serverSocket, validName)
				print "File transfer complete."
				serverSocket.close()

	#this section handles sending the -l command to the server
	elif len(sys.argv) == 5:
		#check if the command is valid
		valid = sendCommand(clientSocket, command)
		if valid == 1:
			#start the dataconnection
			serverSocket = startDataConnection(dataPort)
			print "Recieving directory from "+serverName+":"+str(dataPort)
			#get the directory from the server
			getDirectory(serverSocket)
			serverSocket.close()


	clientSocket.close()

if __name__ == '__main__':
	main()

