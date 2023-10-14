#include <stdio.h>	// Import for `printf` & `perror` functions
#include <stdlib.h>	// Import for `atoi` function
#include <string.h>	// Import for string functions
#include <unistd.h>	// Import for `fork`, `fcntl`, `read`, `write`, `lseek, `_exit` functions
#include <fcntl.h>	// Import for `fcntl` functions
#include <sys/types.h>	// Import for `socket`, `bind`, `listen`, `accept`, `fork`, `lseek` functions
#include <errno.h>	// Import for `errno` variable
#include <sys/socket.h>	// Import for `socket`, `bind`, `listen`, `accept` functions
#include <netinet/in.h>	// Import for `sockaddr_in` stucture
#include <stdbool.h>
void connectionHandler(int sockfd);	// Handles the read & write operations to the server

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

int main()
{
	int sockfd, portno, n;
	struct sockaddr_in serverAddress;
	struct hostent *server; // store info about the given host
	
	char buffer[1024];

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		error("Error opening socket");
	}
	int reuse = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
	    error("setsockopt(SO_REUSEADDR) failed");
	}
	
	portno = 8081;
	//server = gethostbyname(argv[1]);
	if(server == NULL)
	{
		printf("Error, No such host\n");
		exit(0);
	}
	
	//bzero((char*) &serverAddress,sizeof(serverAddress));
	
	
	//bcopy((char*)server->h_addr , (char*)&serverAddress.sin_addr.s_addr , server->h_length); // transfer data about server from server to server_addr
	serverAddress.sin_family = AF_INET;			// IPV4
	serverAddress.sin_port = htons(portno);			// Server will listen to port 8081
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);	// Binds te socket to all interface
	
	if(connect(sockfd,(struct sockaddr *) &serverAddress,sizeof(serverAddress)) < 0 )
		error("Error while connectiong to server");
	
	connectionHandler(sockfd);
	/*while(1)
	{
		n = read(sockfd, &data_len , sizeof(int));
		if(n < 0)
			error("Error on reading");
			
		n = read(sockfd, buffer , sizeof(buffer)-1);
		if(n < 0)
			error("Error on reading");
		buffer[n] = '\0';
		printf("%s\n",buffer);
		
		
		n = read(0,&buffer,sizeof(buffer));
		buffer[n-1] = '\0';
		
		n = write(sockfd , buffer, strlen(buffer));
		if(n < 0)
			error("Error on writing");
		printf("\n %s is sent to server.\n",buffer);
		break;
	}*/
	close(sockfd);
	return 0;
}

void connectionHandler(int sockfd)
{
	char readBuffer[1024], writeBuffer[1024];		// Buffer used for reading from/ writing to the server
	int readBytes, writeBytes;
	
	char tempBuffer[1024];
	
	do
	{
		bzero(readBuffer, sizeof(readBuffer));		// Empty the readBuffer
		bzero(tempBuffer, sizeof(tempBuffer));
		
		readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
		if(readBytes == -1)
		{
			close(sockfd);
			error("Error while reading from server");
		}
		else if(readBytes == 0)
			printf("\nNo data received from server");
		else if(strchr(readBuffer, '^') != NULL)
		{
			// Skip read from client
			strncpy(tempBuffer, readBuffer, (size_t)(strlen(readBuffer) -1));
			//strcpy(tempBuffer, readBuffer);
			printf("\n%s",tempBuffer);
			
			writeBytes = write(sockfd, "^", strlen("^"));
			if(writeBytes == -1)
			{
				close(sockfd);
				error("Error while writing to clint socket");
			}
		}
		else if(strchr(readBuffer, '$') != NULL)
		{
			// Server sent an error message and is now closing its end of connection
			strncpy(tempBuffer, readBuffer, (size_t)(strlen(readBuffer) -2));
			//strcpy(tempBuffer, readBuffer);
			printf("\n%s",tempBuffer);
			printf("\nClosing the connection to the server!!\n");
			break;
		}
		else
		{
			bzero(writeBuffer, sizeof(writeBuffer));	// Empty the write buffer
			
			if(strchr(readBuffer,'#') != NULL)
				strcpy(writeBuffer, getpass(readBuffer));
			else
			{
				printf("\n%s",readBuffer);
				scanf("%[^\n]%*c", writeBuffer); 	// Take input
			}
			
			writeBytes = write(sockfd, writeBuffer, strlen(writeBuffer));
			if(writeBytes == -1)
			{
				perror("Error while writing to client socket");
				printf("\nclosing the connection to the server\n");
				break;	
			}
		}
	}while (readBytes > 0);
	
	close(sockfd);
}
		
		
		
		
		
		
