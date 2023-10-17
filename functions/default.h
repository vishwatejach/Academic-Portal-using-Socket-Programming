#ifndef DEFAULT_FUNCTIONS
#define DEFAULT_FUNCTIONS

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
#include "../Structures/student.h"
#include "../Structures/faculty.h"
#include "./constant.h"

int loginHandler(int type , int sockfd);
bool get_faculty_details(int sockfd, int facultyID);
bool get_student_details(int sockfd, int studentID);

int loginHandler(int type, int sockfd)
{
	int readBytes, writeBytes;			// Number of bytes written to / read from the socket 
	char readBuffer[1024], writeBuffer[1024];	// Buffer for reading from / writing to the client 
	char tempBuffer[1024];
	struct Faculty faculty;
	struct Student student;
	int ID;
	
	bzero(readBuffer, sizeof(readBuffer));
	bzero(writeBuffer, sizeof(writeBuffer));
	
	// Get login message for respective user type
	if(type == 0)
		strcpy(writeBuffer, ADMIN_LOGIN_WELCOME);
	else if(type == 1)
		strcpy(writeBuffer, FACULTY_LOGIN_WELCOME);
	else if(type == 2)
		strcpy(writeBuffer, STUDENT_LOGIN_WELCOME);
	
	// Append the request for LOGIN ID message
	strcat(writeBuffer,"\n");
	strcat(writeBuffer, LOGIN_ID);
	
	writeBytes = write(sockfd, writeBuffer, strlen(writeBuffer));
	if(writeBytes == -1)
	{
		perror("Error writing Welcome & Login to the client");
		return -1;
	}
	
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
	if(readBytes == -1)
	{
		perror("Error reading Login ID to the client");
		return -1;
	}
	printf("\n Login: First Data Received is %s.\n",readBuffer);
	bool userFound = false;
	
	if(type == 0)
	{
		if(strcmp(readBuffer, ADMIN_LOGIN_ID) == 0)
			userFound = true;
	}
	else if(type == 1)
	{
		bzero(tempBuffer,sizeof(tempBuffer));
		strcpy(tempBuffer, strtok(readBuffer, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"));
		ID = atoi(tempBuffer);
		printf("\nFaculty ID : %d\n",ID);
		int facultyFD = open(FACULTY_FILE, O_RDONLY);
		if(facultyFD == -1)
		{
			perror("Error in opening faculty file");
			return -1;
		}
		lseek(facultyFD,0,SEEK_SET);
		off_t offset = lseek(facultyFD, ID * sizeof(struct Faculty),SEEK_SET);
		if(offset >= 0)
		{
			struct flock lock = {F_RDLCK, SEEK_SET, ID * sizeof(struct Faculty), sizeof(struct Faculty), getpid()};
			
			int lockStatus = fcntl(facultyFD, F_SETLKW, &lock);
			if(lockStatus == -1)
			{
				perror("Error in obtaining read lock on faculty record");
				return -1;
			}
			
			readBytes = read(facultyFD, &faculty , sizeof(struct Faculty));
			lock.l_type = F_UNLCK;
			fcntl(facultyFD, F_SETLK, &lock);
			if(readBytes == -1)
			{
				perror("Error while reading faculty data");
				return -1;
			}
			printf("\nUsername : %s\n",readBuffer);
			if(strcmp(faculty.login, readBuffer) == 0)
			{
				userFound = true;	
			}
			close(facultyFD);
		}
		else
		{
			writeBytes =write(sockfd, LOGIN_ID_DOESNT_EXIT, strlen(LOGIN_ID_DOESNT_EXIT));
		}
	}
	else if(type == 2)
	{
		bzero(tempBuffer,sizeof(tempBuffer));
		strcpy(tempBuffer, strtok(readBuffer, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"));
		ID = atoi(tempBuffer);
		
		int studentFD = open(STUDENT_FILE, O_RDONLY);
		if(studentFD == -1)
		{
			perror("Error in opening faculty file");
			return -1;
		}
		lseek(studentFD,0,SEEK_SET);
		off_t offset = lseek(studentFD, ID * sizeof(struct Student),SEEK_SET);
		if(offset >= 0)
		{
			struct flock lock = {F_RDLCK, SEEK_SET, ID * sizeof(struct Student), sizeof(struct Student), getpid()};
			
			int lockStatus = fcntl(studentFD, F_SETLKW, &lock);
			if(lockStatus == -1)
			{
				perror("Error in obtaining read lock on faculty record");
				return -1;
			}
			
			readBytes = read(studentFD, &faculty , sizeof(struct Student));
			lock.l_type = F_UNLCK;
			fcntl(studentFD, F_SETLK, &lock);
			if(readBytes == -1)
			{
				perror("Error while reading faculty data");
				return -1;
			}
			if(student.active == 0)
			{
				userFound = false;
			}
			else if(strcmp(student.login, readBuffer) == 0)
				userFound = true;
			close(studentFD);
		}
		else
		{
			writeBytes =write(sockfd, LOGIN_ID_DOESNT_EXIT, strlen(LOGIN_ID_DOESNT_EXIT));
		}
	}
	
	if(userFound)
	{
		printf("\nUser Found!!\n");
		bzero(writeBuffer, sizeof(writeBuffer));
		writeBytes = write(sockfd, PASSWORD, strlen(PASSWORD));
		if (writeBytes == -1)
		{
		    perror("Error writing PASSWORD message to client!");
		    return -1;
		}
		
		bzero(readBuffer, sizeof(readBuffer));
		readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
		if (readBytes == 1)
		{
		    perror("Error reading password from the client!");
		    return -1;
		}
		
		char hashPassword[1024];
		strcpy(hashPassword, crypt(readBuffer, "MK"));
		printf("\n%s & %s\n",readBuffer,hashPassword);
		if(type == 0)
		{
		    printf("\nChecking for Admin\n");
		    if(strcmp(hashPassword, ADMIN_PASSWORD) == 0)
		    {
		    	printf("\nAdmin cradential matched!\n");
		    	return 0;
		    }
		    else
		    {
		    	return -99;
		    }
		}
		else if(type == 1)
		{
		    printf("\nChecking for Faculty\n");
		    if(strcmp(hashPassword, faculty.password) == 0)
		    {
		    	printf("\nHashPass : %s & faculty Pass : %s\n",hashPassword, faculty.password);
		        return faculty.id;
		    }
		    else
		    {
		    	return -99;
		    }
		}
		else if(type == 2)
		{
		    printf("\nChecking for Student\n");
		    if(strcmp(hashPassword, student.password) == 0)
		    {
		        return student.id;
		    }
		    else
		    {
		    	return -99;
		    }
		}
	}
	else
	{
		bzero(writeBuffer, sizeof(writeBuffer));
        	writeBytes = write(sockfd, INVALID_LOGIN, strlen(INVALID_LOGIN));
	}
	
	return -1;	
}

//-----------------------------------------------------------------------
//--------------------------GET FACULTY DETALIS--------------------------
//-----------------------------------------------------------------------

bool get_faculty_details(int sockfd, int facultyID)
{
    ssize_t readBytes, writeBytes;             // Number of bytes read from / written to the socket
    char readBuffer[1000], writeBuffer[10000]; // A buffer for reading from / writing to the socket
    char tempBuffer[1000];

    struct Faculty faculty;
    int facultyFD;
    struct flock lock = {F_RDLCK, SEEK_SET, 0, sizeof(struct Faculty), getpid()};

    if (facultyID == -1)
    {   
        writeBytes = write(sockfd, GET_FACULTY_ID, strlen(GET_FACULTY_ID));
        if (writeBytes == -1)
        {
            perror("Error while writing GET_FACULTY_ID message to client!");
            return false;
        }

        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error getting faculty ID from client!");
            return false;
        }

        facultyID = atoi(readBuffer);
    }

    facultyFD = open(FACULTY_FILE, O_RDONLY);
    if (facultyFD == -1)
    {
        // faculty File doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, ID_DOESNT_EXIT);
        strcat(writeBuffer, "^");
        writeBytes = write(sockfd, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing ID_DOESNT_EXIT message to client!"); // WRONG
            return false;
        }
        readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    lseek(facultyFD, 0, SEEK_SET);
    int offset = lseek(facultyFD, facultyID * sizeof(struct Faculty), SEEK_SET);
    if (errno == EINVAL)
    {
        // faculty record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, ID_DOESNT_EXIT);
        strcat(writeBuffer, "^");
        writeBytes = write(sockfd, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing ID_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required faculty record!");
        return false;
    }
    lock.l_start = offset;

    int lockStatus = fcntl(facultyFD, F_SETLKW, &lock);
    if (lockStatus == -1)
    {
        perror("Error while obtaining read lock on the faculty file!");
        return false;
    }

    readBytes = read(facultyFD, &faculty, sizeof(struct Faculty));
    lock.l_type = F_UNLCK;
    fcntl(facultyFD, F_SETLK, &lock);
    if (readBytes == 	-1)
    {
        perror("Error reading faculty record from file!");
        return false;
    }
    if(readBytes == 0)
    {
    	// faculty record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, ID_DOESNT_EXIT);
        strcat(writeBuffer, "^");
        writeBytes = write(sockfd, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing ID_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    
//-------------------------------------------------------------------------------------
//-----------------------PRINTING FACULTY DATA-----------------------------------------
//-------------------------------------------------------------------------------------
    bzero(writeBuffer, sizeof(writeBuffer));
    sprintf(writeBuffer, "faculty Details - \n\tID : %d\n\tName : %s\n\tDepartment : %s\n\tDesignation : %s\n\tEmail : %s\n\tAddress : %s\n\tLoginID : %s", faculty.id, faculty.name, faculty.department, faculty.designation, faculty.email, faculty.address, faculty.login);

    strcat(writeBuffer, "\n\nYou'll now be redirected to the main menu...^");

    writeBytes = write(sockfd, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error writing faculty info to client!");
        return false;
    }

    readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read
    return true;
}

//-----------------------------------------------------------------------
//------------------------- GET STUDENT DETALIS--------------------------
//-----------------------------------------------------------------------

bool get_student_details(int sockfd, int studentID)
{
    ssize_t readBytes, writeBytes;             // Number of bytes read from / written to the socket
    char readBuffer[1000], writeBuffer[10000]; // A buffer for reading from / writing to the socket
    char tempBuffer[1000];

    struct Student student;
    int studentFD;
    struct flock lock = {F_RDLCK, SEEK_SET, 0, sizeof(struct Student), getpid()};

    if (studentID == -1)
    {   
        writeBytes = write(sockfd, GET_STUDENT_ID, strlen(GET_STUDENT_ID));
        if (writeBytes == -1)
        {
            perror("Error while writing GET_STUDENT_ID message to client!");
            return false;
        }

        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error getting student ID from client!");
            return false;
        }

        studentID = atoi(readBuffer);
        printf("\nStudent ID: %d\n",studentID);
    }

    studentFD = open(STUDENT_FILE, O_RDONLY);
    if (studentFD == -1)
    {
        // student File doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, ID_DOESNT_EXIT);
        strcat(writeBuffer, "^");
        writeBytes = write(sockfd, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing ID_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    lseek(studentFD, 0, SEEK_SET);
    int offset = lseek(studentFD, studentID * sizeof(struct Student), SEEK_SET);
    if (errno == EINVAL)
    {
        // student record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, ID_DOESNT_EXIT);
        strcat(writeBuffer, "^");
        writeBytes = write(sockfd, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing ID_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required student record!");
        return false;
    }
    lock.l_start = offset;

    int lockStatus = fcntl(studentFD, F_SETLKW, &lock);
    if (lockStatus == -1)
    {
        perror("Error while obtaining read lock on the student file!");
        return false;
    }

    readBytes = read(studentFD, &student, sizeof(struct Student));
    lock.l_type = F_UNLCK;
    fcntl(studentFD, F_SETLK, &lock);
    if (readBytes == -1)
    {
        perror("Error reading student record from file!");
        return false;
    }
    if(readBytes == 0)
    {
    	// student record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, ID_DOESNT_EXIT);
        strcat(writeBuffer, "^");
        writeBytes = write(sockfd, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing ID_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }

//-------------------------------------------------------------------------------------
//-----------------------PRINTING STUDENT DATA-----------------------------------------
//-------------------------------------------------------------------------------------
    bzero(writeBuffer, sizeof(writeBuffer));
    sprintf(writeBuffer, "student Details - \n\tID : MT2023%d\n\tStatus : %d\n\tName : %s\n\tGender : %c\n\tAge: %d\n\tEmail : %s\n\tAddress : %s\n\tLoginID : %s", student.id, student.active, student.name, student.gender, student.age, student.email, student.address, student.login);

    strcat(writeBuffer, "\n\nYou'll now be redirected to the main menu...^");

    writeBytes = write(sockfd, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error writing student info to client!");
        return false;
    }

    readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read
    return true;
}

#endif





























