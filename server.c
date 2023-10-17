#include <stdio.h>	// Import for `printf` & `perror` functions
#include <stdlib.h>	// Import for `atoi` function
#include <string.h>	// Import for string functions
#include <unistd.h>	// Import for `fork`, `fcntl`, `read`, `write`, `lseek, `_exit` functions
#include <fcntl.h>	// Import for `fcntl` functions
#include <sys/types.h>	// Import for `socket`, `bind`, `listen`, `accept`, `fork`, `lseek` functions
#include <errno.h>	// Import for `errno` variable
#include <sys/socket.h>	// Import for `socket`, `bind`, `listen`, `accept` functions
#include <netinet/in.h>	// Import for `sockaddr_in` stucture

#include "./functions/constant.h"
#include "./functions/admin.h"
#include "./functions/faculty.h"
#include "./functions/student.h"

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

void connectionHandler(int confd);	// Handles the communication with the client

int main()
{	
	int sockfd, newsockfd, portno, n;
	int choice, data_len = 0;
	int menu_no = 0;
	char buffer[1024];
	
	struct sockaddr_in serv_addr, cli_addr; // gives internet address
	socklen_t clilen; // data type to store : 32 bit 
	
	sockfd = socket(AF_INET , SOCK_STREAM , 0);
	if(sockfd < 0)
	{
		error("Error Opening Socket.");
	}
	printf("\nSocket created successfully!!");
	
	bzero((char*) &serv_addr, sizeof(serv_addr));
	portno = 8081;
	
	serv_addr.sin_family = AF_INET;		// IPv4
	serv_addr.sin_addr.s_addr = INADDR_ANY;	// Server will listen to port 8080
	serv_addr.sin_port = htons(portno);	// Server will listen to port 8080
	
	if(bind(sockfd,(struct sockaddr *)&serv_addr , sizeof(serv_addr)) < 0)
	{
		error("Binding Failed");
	}
	printf("\nSocket Bind successfully at port number %d.",portno);
	listen(sockfd , 5);			// Listen to max 5 users at a time
	clilen = (int)sizeof(cli_addr);
	
	

	while(1)
	{
		newsockfd = accept(sockfd , (struct sockaddr *)&cli_addr , &clilen);	// For multiple cilent handling
		if(newsockfd < 0)
			error("Error while listening for connection on the server!!");
		else
		{
			if(!fork())
			{
				// Child
				connectionHandler(newsockfd);
				close(newsockfd);
				exit(0);
			}
		}
	}
	if(close(sockfd) == -1) 
	{
        	perror("Error closing socket");
        	return 1;
    	}
	printf("\nSocket closed !!\n");
	return 0;
}

void connectionHandler(int sockfd)
{
	printf("\nClent has connected to the server!!");
	
	char readBuffer[1024], writeBuffer[1024];
	int readBytes, writeBytes, choice;
	
	writeBytes = write(sockfd, LOGIN_MENU, strlen(LOGIN_MENU));
	if(writeBytes == -1)
			error("Error while sending  Menu to user !!");
	else
	{
		bzero(readBuffer, sizeof(readBuffer));
		readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
		if(readBytes == -1)
			error("Error while reading from client");
		else if(readBytes == 0)
			printf("No data was sent by the client");
		else
		{
			choice = atoi(readBuffer);
			switch(choice)
			{
				case 1:
					// User is Admin
					adminHandler(sockfd);
					break;
				case 2:
					// User is Faculty
					facultyHandler(sockfd);
					break;
				case 3:
					// User is Student
					studentHandler(sockfd);
					break;
				default:
					break;
			}
		}
	}
	printf("\nTerminating connection to client!!");
}


