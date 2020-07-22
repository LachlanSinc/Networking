# Author: Lachlan Sinclair
# Program 1: Chatserve
# Description: Chat program using a client server connection, via a 
# TCP protocol connection
# CS 372: Introduction to Computer Networks
# Last Modified: 11/3/2019

#Citations: CS372 lecture 15, python code for the server side
# https://docs.python.org/release/2.6.5/library/internet.html

import sys
from socket import *


#hard coded handle to be used in all communication
serverHandle = "Lachlan> "


# Function: startup
# Pre-conditions: user entered port number when starting up the program
# Post-condistions: the socket is set up
# Description: Sets up and returns the server socket
def startUp(portNum):
	
	serverSocket = socket(AF_INET, SOCK_STREAM)
	serverSocket.bind(('', portNum))
	serverSocket.listen(1)
	print 'Chatserve is now running.'
	return serverSocket


# Function: sendMessage
# Pre-conditions: socket is actively connected to a client
# Post-condistions: sends message using the socket, returns the quit bool
# Description: prompts the user for input and sends that input over the socket
def sendMessage(connectionSocket):

	message = raw_input(serverHandle) #prompt and read in user input
	
	if len(message)==0: #handle entering jsut a carriage return
		return 2

	connectionSocket.send(message) #send the user input over the socket

	if "\quit" == message:  #check if the entered message is \quit
		return 1 	#return true

	return 0 #return false


# Function: recieveMessage
# Pre-conditions: the socket is actively connected to client
# Post-condistions: a message is read from the socket and displayed on the console, returns the quit bool
# Description: Waits for a message from the client, then displays the message
def recieveMessage(connectionSocket, userHandle):

	sentence = connectionSocket.recv(501) #wait for a message from the client, read it into message

	if "\quit" == sentence: #check to see if the message is \quit
		return 1 	#return true wihout printing the message


	print(userHandle + sentence) #print the users handle and the message
	return 0 #return false


def main():

	if len(sys.argv) != 2:
		print("USAGE: python chatclient.py portnumber")
		exit(1)

	portNumber = int(sys.argv[1])#convert the portnumber entered by the user to a int
	
	serverSocket = startUp(portNumber) #call the function to set up the socket

	#infinite loop until sigint
	while 1:
		connectionSocket, addr = serverSocket.accept() #accept a new connection

		userHandle = connectionSocket.recv(501) #read in the users handle
		connectionSocket.send(serverHandle)	#send the servers handle

		#loop in the connection until \quit is recieved or entered
		while 1:

			quitBool = recieveMessage(connectionSocket, userHandle) #wait for a message, update the quitbool
			if quitBool==1:
				break #break out of the loop and wait for a new conenction
		
			quitBool = sendMessage(connectionSocket) #send a message to the client, update the quitbool
			
			while quitBool==2: #while the user just loop waiting for a message
				print("Please enter a message or \\quit to quit.")
				quitBool = sendMessage(connectionSocket) 
			
			if quitBool==1:
				break #break out of the loop and wait for a new connection
						
		connectionSocket.close() #close the connection

#enter the main method
if __name__ == '__main__':
	main()





















