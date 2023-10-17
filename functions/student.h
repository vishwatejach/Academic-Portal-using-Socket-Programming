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
#include "./constant.h"
#include "./default.h"
#include "../Structures/student.h"
#include "../Structures/course.h"
#include "../Structures/faculty.h"

#ifndef STUDENT_FUNCTIONS
#define STUDENT_FUNCITONS

//bool offeringCoursesStudent(int sockfd);
bool addStudentCourses(int sockfd,int studentID);
bool dropCourse(int sockfd);
bool enrolledCourse(int sockfd,int student);
bool changeStudentPassword(int sockfd,int studentID);

bool studentHandler(int sockfd)
{
	int studentID;
	if((studentID = loginHandler(2,sockfd)) != -1)
	{
		if(studentID == -99)
		{
			// If Username or password didnt match
			printf("\n Data not found !! -99\n");
			write(sockfd, LOGOUT, strlen(LOGOUT));
			return false;
				
		}
		printf("\nInto student.h\n");
		int readBytes, writeBytes;			// Number of bytes written to / read from the socket 
		char readBuffer[1024], writeBuffer[1024];	// Buffer for reading from / writing to the client 
		bzero(writeBuffer, sizeof(writeBuffer));
		strcpy(writeBuffer,"Welcome Student!!");
		
		while(1)
		{
			strcat(writeBuffer, "\n");
			strcat(writeBuffer, STUDENT_MENU);
			writeBytes = write(sockfd, writeBuffer, strlen(writeBuffer));
			if (writeBytes == -1)
			{
				perror("Error while writing STUDENT_MENU to client!");
				return false;
			}
			
			bzero(writeBuffer, sizeof(writeBuffer));
			
			readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
			if (readBytes == -1)
			{
				perror("Error while reading client's choice for ADMIN_MENU");
				return false;
			}
			
			int choice = atoi(readBuffer);
			printf("\n Student Choice is %d",choice);
			switch(choice)
			{
				case 1:
					//offeringCoursesStudent(sockfd);
					break;
				case 2:
					//addStudentCourses(sockfd);
					break;
				case 3:
					//dropCourse(sockfd);
					break;
				case 4:
					//enrolledCourse(sockfd);
					break;
				case 5:
					changeStudentPassword(sockfd, studentID);
					break;
				default:
					writeBytes = write(sockfd, LOGOUT, strlen(LOGOUT));
                			return false;
			}
		}
	}
	else
	{
		// Student LOGIN FAILED
        	return false;
	}
	return true;
}

// ---------------------------------------------------------------------------------------------
// ------------------------------ OFFERS COURSES -----------------------------------------------
// ---------------------------------------------------------------------------------------------


// ---------------------------------------------------------------------------------------------
// ---------------------------- ADD Courses ----------------------------------------------------
// ---------------------------------------------------------------------------------------------
bool addStudentCourses(int sockfd, int studentID)
{
	int readBytes, writeBytes;
	char readBuffer[1024], writeBuffer[1024];
	
	struct Student student;
	struct Course course;
	off_t offset;
	int lockstatus,choice;
	
	//-----------------------------------------------------------------
	//-------------------------------- MENU ---------------------------
	//-----------------------------------------------------------------
	writeBytes = write(sockfd, STUDENT_ADD_COURSE_MENU, strlen(STUDENT_ADD_COURSE_MENU));
	if (writeBytes == -1)
	{
		perror("Error while writing STUDENT_ADD_COURSE_MENU message to client!");
		return false;
	}
	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
	if (readBytes == -1)
	{
		perror("Error while reading student ID from client!");
		return false;
	}
	choice = atoi(readBuffer);
	
	
	int studentFD = open(STUDENT_FILE, O_RDONLY);
	if(studentFD == -1)
	{
		perror("Error while openeing student file");
		return false;
	}
	else
	{
		offset = lseek(studentFD,-sizeof(struct Student), SEEK_END);
		if(offset == -1)
		{
			perror("Error seeking to last student record");
			return false;
		}
		
		struct flock lock = {F_RDLCK,SEEK_SET,offset,sizeof(struct Student),getpid()};
		int lockstatus = fcntl(studentFD, F_SETLKW, &lock);
		if(lockstatus == -1)
		{
			perror("Error while obtaining read lock");
			return false;
		}
		
		readBytes = read(studentFD,&student,sizeof(struct Student));
		lock.l_type = F_UNLCK;
		fcntl(studentFD,F_SETLK,&lock);
		if(readBytes == -1)
		{
			perror("Error while reading student record from file");
			return false;
		}
		
		
		
		
	}
	
}

// ---------------------------------------------------------------------------------------------
// ---------------------------- CHANGE PASSWORD ------------------------------------------------
// ---------------------------------------------------------------------------------------------
bool changeStudentPassword(int sockfd,int studentID)
{
	int readBytes, writeBytes;
	char readBuffer[1024], writeBuffer[1024];
	
	struct Student student;
	printf("\nInsidee changePassword for student\n");
	off_t offset;
	int lockstatus;
	
	int studentFD = open(STUDENT_FILE, O_RDONLY);
	if(studentFD == -1)
	{
		perror("Error while openeing student file");
		return false;
	}
	lseek(studentFD,0,SEEK_SET);
	offset = lseek(studentFD, studentID * sizeof(struct Student), SEEK_SET);
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
	
	
	//-----------------------------------------------------------------
	//----------------------LOCK REQUIRED DATA-------------------------
	//-----------------------------------------------------------------
	struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Student), getpid()};
	lockstatus = fcntl(studentFD, F_SETLKW, &lock);
	if(lockstatus == -1)
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
	close(studentFD);
	printf("\nStudent password is %s\n",student.password);
	//-----------------------------------------------------------------
	//---------------------- CHANGE PASSWORD --------------------------
	//-----------------------------------------------------------------
	bzero(writeBuffer, sizeof(writeBuffer));
	writeBytes = write(sockfd, CHANGE_PASSWORD, strlen(CHANGE_PASSWORD));
	if (writeBytes == -1)
	{
		perror("Error while writing CHANGE_PASSWORD message to client!");
		return false;
	}
	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
	if (readBytes == -1)
	{
		perror("Error while reading  updated password of course from client!");
		return false;
	}
	
	char hashedPassword[1000];
	strcpy(student.password, crypt(readBuffer, "MK"));
	printf("\nPassword is %s\n",student.password);
	
	// --------------------------Writing back --------------------------------
	studentFD = open(STUDENT_FILE, O_WRONLY);
	if(studentFD == -1)
	{
		perror("Error while opening student file");
        	return false;
	}
	lseek(studentFD,0,SEEK_SET);
	offset = lseek(studentFD, studentID * sizeof(struct Student), SEEK_SET);
	if(offset == -1)
	{
		perror("Error while seeking to required student record");
		return false;
	}
	
	lock.l_type = F_WRLCK;
	lock.l_start = offset;
	lockstatus = fcntl(studentFD, F_SETLK, &lock);
	if(lockstatus == -1)
	{
		perror("Error while obtaining write lock on student record");
		return false;
	}
	
	writeBytes = write(studentFD, &student, sizeof(struct Student));
	lock.l_type = F_UNLCK;
	fcntl(studentFD, F_SETLK, &lock);
	if(writeBytes == -1)
	{
		perror("Error while writing update to student file");
		return false;
	}
	
	
	close(studentFD);
	
	writeBytes = write(sockfd, STUDENT_MOD_SUCCESS, strlen(STUDENT_MOD_SUCCESS));
	if (writeBytes == -1)
	{
		perror("Error while writing STUDENT_MOD_SUCCESS message to client!");
		return false;
	}
	
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read
	
}

#endif
