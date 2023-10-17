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

#ifndef FACULTY_FUNCTIONS
#define FACULTY_FUNCITONS

bool addCourses(int sockfd,int facultyID);
bool changePassword(int sockfd,int facultyID);
bool removeCourse(int sockfd,int facultyID);
bool updateCourse(int sockfd,int facultyID);
bool offeringCourses(int sockfd,int facultyID);

bool facultyHandler(int sockfd)
{
	int facultyID;
	if((facultyID = loginHandler(1,sockfd)) != -1)
	{
		if(facultyID == -99)
		{
			// If Username or password didnt match
			printf("\n Data not found !! -99\n");
			write(sockfd, LOGOUT, strlen(LOGOUT));
			return false;
				
		}
		printf("\nInto faculty.h\n");
		int readBytes, writeBytes;			// Number of bytes written to / read from the socket
		char readBuffer[1024], writeBuffer[1024];	// Buffer for reading from / writing to the client 
		bzero(writeBuffer, sizeof(writeBuffer));
		strcpy(writeBuffer,"Welcome Faculty!!"); 
		
		while(1)
		{
			strcat(writeBuffer, "\n");
			strcat(writeBuffer, FACULTY_MENU);
			writeBytes = write(sockfd, writeBuffer, strlen(writeBuffer));
			if (writeBytes == -1)
			{
				perror("Error while writing FACULTY_MENU to client!");
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
			printf("\n Faculty Choice is %d",choice);
			switch(choice)
			{
				case 1:
					offeringCourses(sockfd,facultyID);
					break;
				case 2:
					addCourses(sockfd,facultyID);
					break;
				case 3:
					removeCourse(sockfd,facultyID);
					break;
				case 4:
					updateCourse(sockfd,facultyID);
					break;
				case 5:
					changePassword(sockfd,facultyID);
					break;
				default:
					writeBytes = write(sockfd, LOGOUT, strlen(LOGOUT));
                			return false;
			}
		}
	}
	else
	{
		// Faculty LOGIN FAILED
        	return false;
	}
	return true;
}

// ---------------------------------------------------------------------------------------------
// ---------------------------- ADD COURSE -----------------------------------------------------
// ---------------------------------------------------------------------------------------------
bool addCourses(int sockfd,int facultyID)
{
	int readBytes, writeBytes;
	char readBuffer[1024], writeBuffer[1024];
	
	struct Course newCourse, oldCourse;
	struct Faculty faculty;
	
	off_t offset;
	int lockstatus;
	
	int facultyFD = open(FACULTY_FILE, O_RDONLY);
	if(facultyFD == -1)
	{
		perror("Error while openeing faculty file");
		return false;
	}
	lseek(facultyFD,0,SEEK_SET);
	offset = lseek(facultyFD, facultyID * sizeof(struct Faculty), SEEK_SET);
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
	
	
	//-----------------------------------------------------------------
	//----------------------LOCK REQUIRED DATA-------------------------
	//-----------------------------------------------------------------
	struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Faculty), getpid()};
	lockstatus = fcntl(facultyFD, F_SETLKW, &lock);
	if(lockstatus == -1)
	{
		perror("Error while obtaining read lock on the faculty file!");
        	return false;
	}
	readBytes = read(facultyFD, &faculty, sizeof(struct Faculty));
	lock.l_type = F_UNLCK;
	fcntl(facultyFD, F_SETLK, &lock);
	if (readBytes == -1)
	{
		perror("Error reading faculty record from file!");
		return false;
	}
	close(facultyFD);
	
	
	//-----------------------------------------------------------------
	//---------------------------- courseID ---------------------------
	//-----------------------------------------------------------------
	int courseFD = open(COURSE_FILE, O_RDONLY);
	if(courseFD == -1 && errno == ENOENT)		// ENOENT : represents the error code for "No such file or directory"
	{
		// course file was never created
		newCourse.id = 0;
	}
	else if(courseFD == -1)
	{
		perror("Error while openeing course file");
		return false;
	}
	else	
	{
		int offset = lseek(courseFD,-sizeof(struct Course), SEEK_END);
		if(offset == -1)
		{
			perror("Error seeking to last course record");
			return false;
		}
		
		struct flock lock = {F_RDLCK,SEEK_SET,offset,sizeof(struct Course),getpid()};
		int lockstatus = fcntl(courseFD, F_SETLKW, &lock);
		if(lockstatus == -1)
		{
			perror("Error while obtaining read lock");
			return false;
		}
		
		readBytes = read(courseFD,&oldCourse,sizeof(struct Course));
		if(readBytes == -1)
		{
			perror("Error while reading course record from file");
			return false;
		}
		
		lock.l_type = F_UNLCK;
		fcntl(courseFD,F_SETLK,&lock);
		
		close(courseFD);
		
		newCourse.id = oldCourse.id + 1;
	}
	
	//-----------------------------------------------------------------
	//-------------------------------- NAME ---------------------------
	//-----------------------------------------------------------------
	bzero(writeBuffer, sizeof(writeBuffer));
	writeBytes = write(sockfd, FACULTY_COURSE_NAME, strlen(FACULTY_COURSE_NAME));
	if (writeBytes == -1)
	{
		perror("Error while writing FACULTY_COURSE_NAME message to client!");
		return false;
	}
	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
	if (readBytes == -1)
	{
		perror("Error while reading  name of course from client!");
		return false;
	}
	strcpy(newCourse.name, readBuffer);
	
	//-----------------------------------------------------------------
	//-------------------------- DEPARTMENT ---------------------------
	//-----------------------------------------------------------------
	bzero(writeBuffer, sizeof(writeBuffer));
	writeBytes = write(sockfd, FACULTY_COURSE_DEPARTMENT, strlen(FACULTY_COURSE_DEPARTMENT));
	if (writeBytes == -1)
	{
		perror("Error while writing FACULTY_COURSE_DEPARTMENT message to client!");
		return false;
	}
	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
	if (readBytes == -1)
	{
		perror("Error while reading  department of course from client!");
		return false;
	}
	strcpy(newCourse.department, readBuffer);
	
	//-----------------------------------------------------------------
	//-------------------------- SEATS --------------------------------
	//-----------------------------------------------------------------
	bzero(writeBuffer, sizeof(writeBuffer));
	writeBytes = write(sockfd, FACULTY_COURSE_SEATS, strlen(FACULTY_COURSE_SEATS));
	if (writeBytes == -1)
	{
		perror("Error while writing FACULTY_COURSE_SEATS message to client!");
		return false;
	}
	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
	if (readBytes == -1)
	{
		perror("Error while reading  seats of course from client!");
		return false;
	}
	newCourse.totalSeats = atoi(readBuffer);
	
	//-----------------------------------------------------------------
	//-------------------------- CREDIT -------------------------------
	//-----------------------------------------------------------------
	bzero(writeBuffer, sizeof(writeBuffer));
	writeBytes = write(sockfd, FACULTY_COURSE_CREDIT, strlen(FACULTY_COURSE_CREDIT));
	if (writeBytes == -1)
	{
		perror("Error while writing FACULTY_COURSE_CREDIT message to client!");
		return false;
	}
	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
	if (readBytes == -1)
	{
		perror("Error while reading  credits of course from client!");
		return false;
	}
	newCourse.credit = atoi(readBuffer);
	
	newCourse.enrolledStudent = 0;
	newCourse.facultyID = facultyID;
	
	//----------------------- Writing data to the file --------------------------------
	courseFD = open(COURSE_FILE, O_CREAT | O_APPEND | O_WRONLY, 0666);
	if(courseFD == -1)
	{
		perror("Error while creating/opening course file");
		return false;
	}
	
	writeBytes = write(courseFD, &newCourse, sizeof(newCourse));
	if (writeBytes == -1)
	{
		perror("Error while writing Course record to file!");
		return false;
	}
	
	close(courseFD);
	
	writeBytes = write(sockfd, FACULTY_MOD_SUCCESS, strlen(FACULTY_MOD_SUCCESS));
	if (writeBytes == -1)
	{
		perror("Error while writing FACULTY_MOD_SUCCESS message to client!");
		return false;
	}
	
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read
}

// ---------------------------------------------------------------------------------------------
// ------------------------------ VIEW COURSE --------------------------------------------------
// ---------------------------------------------------------------------------------------------
bool offeringCourses(int sockfd,int facultyID)
{
	ssize_t readBytes, writeBytes;             // Number of bytes read from / written to the socket
    char readBuffer[1000], writeBuffer[10000]; // A buffer for reading from / writing to the socket
    char tempBuffer[1000];
    
    int courseID;
    struct Course course;
    int courseFD;
    
    struct flock lock = {F_RDLCK, SEEK_SET, 0, 0, getpid()};
    
    courseFD = open(COURSE_FILE, O_RDONLY);
    if(courseFD == -1)
    {
    	// course File doesn't exist
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
    // Getting lock
    int lockstatus = fcntl(courseFD, F_SETLKW, &lock);
    if (lockstatus == -1)
    {
        perror("Error while obtaining read lock on the course file!");
        return false;
    }
    char data[1024];
    bzero(data,sizeof(data));
    strcat(data,"Courses list :");
    while(read(courseFD, &course, sizeof(struct Course)) > 0)
    {
    	if(course.facultyID == facultyID)
    	{
    		char temp[500];
    		bzero(temp,sizeof(temp));
    		sprintf(temp,"\nID : %d\nName: %s\nDepartment: %s\nTotal Seats : %d\nCredit : %d\n",course.id,course.name,course.department,course.totalSeats,course.credit);
        	strcat(data,temp);
    	}
    }
    strcat(data,"\n\nYou'll now be redirected to the main menu...^");
//-------------------------------------------------------------------------------------
//---------------------- PRINTING COURSE DATA------------------------------------------
//-------------------------------------------------------------------------------------
    
    writeBytes = write(sockfd, data, strlen(data));
    if (writeBytes == -1)
    {
        perror("Error writing course name to client!");
        return false;
    }

    readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read
    return true;
}

// ---------------------------------------------------------------------------------------------
// ---------------------------- MODIFY COURSE --------------------------------------------------
// ---------------------------------------------------------------------------------------------
bool updateCourse(int sockfd,int courseID)
{
	int readBytes, writeBytes;
	char readBuffer[1024], writeBuffer[1024];
	
	struct Course course;
	
	off_t offset;
	int lockstatus, searchID = -1;
	
	//-----------------------------------------------------------------
	//---------------------------------- ID ---------------------------
	//-----------------------------------------------------------------
	writeBytes = write(sockfd, FACULTY_MOD_COURSE_ID, strlen(FACULTY_MOD_COURSE_ID));
	if (writeBytes == -1)
	{
		perror("Error while writing FACULTY_MOD_COURSE_ID message to client!");
		return false;
	}
	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
	if (readBytes == -1)
	{
		perror("Error while reading course ID from client!");
		return false;
	}
	courseID = atoi(readBuffer);
	
	int courseFD = open(COURSE_FILE, O_RDONLY);
	if(courseFD == -1)
	{
		// course file doesnt exist
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
		
	if (errno == EINVAL)
	{
		// course record doesn't exist
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
	
	
	//-----------------------------------------------------------------
	//----------------------LOCK REQUIRED DATA-------------------------
	//-----------------------------------------------------------------
	struct flock lock = {F_RDLCK, SEEK_SET, 0, 0, getpid()};
	lockstatus = fcntl(courseFD, F_SETLKW, &lock);
	if(lockstatus == -1)
	{
		perror("Error while obtaining read lock on the course file!");
        	return false;
	}
	lseek(courseFD,0,SEEK_SET);
	while(readBytes = read(courseFD, &course, sizeof(struct Course)) > 0)
	{
		searchID++;
		if(course.id == courseID)
		{
			lock.l_type = F_UNLCK;
			lockstatus = fcntl(courseFD, F_SETLK, &lock);
			if(lockstatus == -1)
			{
				perror("Error while unlocking read lock on the course file!");
				return false;
			}
			break;	
		}
	}
	if(searchID != -1)
	printf("\nData Found at %d index.\n",searchID);
	else
	printf("\nData Not Found\n");
	if(readBytes == 0)
	{
		// course record doesn't exist
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
	close(courseFD);
	
	//-----------------------------------------------------------------
	//-------------------------------- MENU ---------------------------
	//-----------------------------------------------------------------
	writeBytes = write(sockfd, FACULTY_MOD_COURSE_MENU, strlen(FACULTY_MOD_COURSE_MENU));
	if (writeBytes == -1)
	{
		perror("Error while writing FACULTY_MOD_COURSE_MENU message to client!");
		return false;
	}
	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
	if (readBytes == -1)
	{
		perror("Error while reading  choice for MODIFI MENU from client!");
		return false;
	}
	
	int choice = atoi(readBuffer);
	if(choice == 0)
	{
		// Non Numberic string was passed 
		writeBytes = write(sockfd, ERRON_INPUT_FOR_NUMBER, strlen(ERRON_INPUT_FOR_NUMBER));
		if (writeBytes == -1)
		{
		    perror("Error while writing ERRON_INPUT_FOR_NUMBER message to client!");
		    return false;
		}
		bzero(readBuffer, sizeof(readBuffer));
		readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read
		return false;
	}
	bzero(writeBuffer, sizeof(writeBuffer));
	switch(choice)
	{
		case 1:
			writeBytes = write(sockfd, ADMIN_MOD_NEW_NAME, strlen(ADMIN_MOD_NEW_NAME));
			if(writeBytes == -1)
			{
				perror("Error while writing ADMIN_MOD_NEW_NAME message to client!");
            			return false;
			}
			bzero(readBuffer, sizeof(readBuffer));
			readBytes = read(sockfd, &readBuffer, sizeof(readBuffer));
			if (readBytes == -1)
			{
			    perror("Error while getting response for course's new name from client!");
			    return false;
			}
			strcpy(course.name, readBuffer);
			break;
		case 2:
			writeBytes = write(sockfd, ADMIN_MOD_NEW_DEPARTMENT, strlen(ADMIN_MOD_NEW_DEPARTMENT));
			if(writeBytes == -1)
			{
				perror("Error while writing ADMIN_MOD_NEW_DEPARTMENT message to client!");
            			return false;
			}
			bzero(readBuffer, sizeof(readBuffer));
			readBytes = read(sockfd, &readBuffer, sizeof(readBuffer));
			if (readBytes == -1)
			{
			    perror("Error while getting response for course's new department from client!");
			    return false;
			}
			strcpy(course.department, readBuffer);
			break;
		case 3:
			writeBytes = write(sockfd, FACULTY_MOD_NEW_SEATS, strlen(FACULTY_MOD_NEW_SEATS));
			if(writeBytes == -1)
			{
				perror("Error while writing FACULTY_MOD_NEW_SEATS message to client!");
            			return false;
			}
			bzero(readBuffer, sizeof(readBuffer));
			readBytes = read(sockfd, &readBuffer, sizeof(readBuffer));
			if (readBytes == -1)
			{
			    perror("Error while getting response for course's new department from client!");
			    return false;
			}
			course.totalSeats = atoi(readBuffer);
			break;
		case 4:
			writeBytes = write(sockfd, FACULTY_MOD_NEW_CREDIT, strlen(FACULTY_MOD_NEW_CREDIT));
			if(writeBytes == -1)
			{
				perror("Error while writing FACULTY_MOD_NEW_CREDIT message to client!");
            			return false;
			}
			bzero(readBuffer, sizeof(readBuffer));
			readBytes = read(sockfd, &readBuffer, sizeof(readBuffer));
			if (readBytes == -1)
			{
			    perror("Error while getting response for course's new department from client!");
			    return false;
			}
			course.credit = atoi(readBuffer);
			break;
		default:
			bzero(writeBuffer, sizeof(writeBuffer));
			strcpy(writeBuffer, INVALID_MENU_CHOICE);
			writeBytes = write(sockfd, writeBuffer, strlen(writeBuffer));
			if (writeBytes == -1)
			{
			    perror("Error while writing INVALID_MENU_CHOICE message to client!");
			    return false;
			}
			readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read
			return false;
	}
	
	// --------------------------Writing back --------------------------------
	
	courseFD = open(COURSE_FILE, O_WRONLY);
	if(courseFD == -1)
	{
		perror("Error while opening course file");
        	return false;
	}
	lseek(courseFD,0,SEEK_SET);
	offset = lseek(courseFD, courseID * sizeof(struct Course), SEEK_SET);
	if(offset == -1)
	{
		perror("Error while seeking to required course record");
		return false;
	}
	
	lock.l_type = F_WRLCK;
	lock.l_start = offset;
	lockstatus = fcntl(courseFD, F_SETLK, &lock);
	if(lockstatus == -1)
	{
		perror("Error while obtaining write lock on course record");
		return false;
	}
	
	writeBytes = write(courseFD, &course, sizeof(struct Course));
	if(writeBytes == -1)
	{
		perror("Error while writing update to course file");
		return false;
	}
	
	lock.l_type = F_UNLCK;
	lockstatus = fcntl(courseFD, F_SETLK, &lock);
	if(lockstatus == -1)
	{
		perror("Error while releasing write lock on course record");
		return false;
	}
	
	close(courseFD);
	
	writeBytes = write(sockfd, ADMIN_MOD_SUCCESS, strlen(ADMIN_MOD_SUCCESS));
	if(writeBytes == -1)
	{
		perror("Error while  ADMIN_MOD_SUCCESS");
		return false;
	}
	
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read

    	return true;
	
}

// ---------------------------------------------------------------------------------------------
// ---------------------------- CHANGE PASSWORD ------------------------------------------------
// ---------------------------------------------------------------------------------------------
bool changePassword(int sockfd,int facultyID)
{
	int readBytes, writeBytes;
	char readBuffer[1024], writeBuffer[1024];
	
	struct Faculty faculty;
	printf("\nInsidee changePassword for faculty\n");
	off_t offset;
	int lockstatus;
	
	int facultyFD = open(FACULTY_FILE, O_RDONLY);
	if(facultyFD == -1)
	{
		perror("Error while openeing faculty file");
		return false;
	}
	lseek(facultyFD,0,SEEK_SET);
	offset = lseek(facultyFD, facultyID * sizeof(struct Faculty), SEEK_SET);
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
	
	
	//-----------------------------------------------------------------
	//----------------------LOCK REQUIRED DATA-------------------------
	//-----------------------------------------------------------------
	struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Faculty), getpid()};
	lockstatus = fcntl(facultyFD, F_SETLKW, &lock);
	if(lockstatus == -1)
	{
		perror("Error while obtaining read lock on the faculty file!");
        	return false;
	}
	readBytes = read(facultyFD, &faculty, sizeof(struct Faculty));
	lock.l_type = F_UNLCK;
	fcntl(facultyFD, F_SETLK, &lock);
	if (readBytes == -1)
	{
		perror("Error reading faculty record from file!");
		return false;
	}
	close(facultyFD);
	printf("\nFaculty password is %s\n",faculty.password);
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
	strcpy(faculty.password, crypt(readBuffer, "MK"));
	printf("\nPassword is %s\n",faculty.password);
	
	// --------------------------Writing back --------------------------------
	facultyFD = open(FACULTY_FILE, O_WRONLY);
	if(facultyFD == -1)
	{
		perror("Error while opening faculty file");
        	return false;
	}
	lseek(facultyFD,0,SEEK_SET);
	offset = lseek(facultyFD, facultyID * sizeof(struct Faculty), SEEK_SET);
	if(offset == -1)
	{
		perror("Error while seeking to required faculty record");
		return false;
	}
	
	lock.l_type = F_WRLCK;
	lock.l_start = offset;
	lockstatus = fcntl(facultyFD, F_SETLK, &lock);
	if(lockstatus == -1)
	{
		perror("Error while obtaining write lock on faculty record");
		return false;
	}
	
	writeBytes = write(facultyFD, &faculty, sizeof(struct Faculty));
	lock.l_type = F_UNLCK;
	fcntl(facultyFD, F_SETLK, &lock);
	if(writeBytes == -1)
	{
		perror("Error while writing update to faculty file");
		return false;
	}
	
	close(facultyFD);
	
	writeBytes = write(sockfd, FACULTY_MOD_SUCCESS, strlen(FACULTY_MOD_SUCCESS));
	if (writeBytes == -1)
	{
		perror("Error while writing FACULTY_MOD_SUCCESS message to client!");
		return false;
	}
	
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read
}

// ---------------------------------------------------------------------------------------------
// ---------------------------- Remove Course ------------------------------------------------
// ---------------------------------------------------------------------------------------------
bool removeCourse(int sockfd,int facultyID)
{
	int readBytes, writeBytes;
	char readBuffer[1024], writeBuffer[1024];
	
	struct Course course;
	int found, searchID = -1;
	int offset;
	
	//-----------------------------------------------------------------
	//---------------------------------- ID ---------------------------
	//-----------------------------------------------------------------
	writeBytes = write(sockfd, FACULTY_MOD_COURSE_ID, strlen(FACULTY_MOD_COURSE_ID));
	if (writeBytes == -1)
	{
		perror("Error while writing FACULTY_MOD_COURSE_ID message to client!");
		return false;
	}
	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
	if (readBytes == -1)
	{
		perror("Error while reading faculty ID from client!");
		return false;
	}
	int courseID = atoi(readBuffer);
	printf("\nUser want to delete %d course\n",courseID);
	int courseFD = open(COURSE_FILE, O_RDWR);
	if(courseFD == -1)
	{
		// Course file doesnt exist
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
	
	//-----------------------------------------------------------------
	//----------------- LOCK REQUIRED DATA FOR WRITE ------------------
	//-----------------------------------------------------------------
	struct flock lock = {F_RDLCK, SEEK_SET, 0, 0, getpid()};
	int lockstatus = fcntl(courseFD, F_SETLKW, &lock);
	if(lockstatus == -1)
	{
		perror("Error while obtaining read lock on the course file!");
        	return false;
	}
	lseek(courseFD,0,SEEK_SET);
	while(readBytes = read(courseFD, &course, sizeof(struct Course)) > 0)
	{
		searchID++;
		if(course.id == courseID && course.facultyID == facultyID)
		{ 
			lock.l_type = F_UNLCK;
			lockstatus = fcntl(courseFD, F_SETLK, &lock);
			if(lockstatus == -1)
			{
				perror("Error while unlocking read lock on the course file!");
				lock.l_type = F_UNLCK;
				lockstatus = fcntl(courseFD, F_SETLK, &lock);
				if(lockstatus == -1)
				{
					perror("Error while unlocking read lock on the course file!");
					return false;
				}
				return false;
			}
			break;	
		}
	}
	lock.l_type = F_UNLCK;
	lockstatus = fcntl(courseFD, F_SETLK, &lock);
	if(lockstatus == -1)
	{
		perror("Error while unlocking read lock on the course file!");
		return false;
	}
	else
	printf("\n Read Lock released\n");
	
	// If user try to delete without adding any courses
	if(readBytes == 0)
	{
		// course record doesn't exist
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
	
	if(searchID != -1)
	{
		printf("\nData Found at %d index.",searchID);
		int offset = lseek(courseFD, (searchID) * sizeof(struct Course), SEEK_SET);
		//printf("\nOffset is %d",offset);
		lock.l_type = F_WRLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start = offset;
		lock.l_len = 0;
		lockstatus = fcntl(courseFD, F_SETLK, &lock);
		if(lockstatus == -1)
		{
			perror("Error while obtaining write lock on the course file!");
			close(courseFD);
			return false;
		}
		offset = lseek(courseFD, offset, SEEK_SET);
		while((read(courseFD, &course, sizeof(struct Course))) > 0)
		{
			readBytes = read(courseFD, &course, sizeof(struct Course));
			if(readBytes > 0)
			{
				printf("\n Greater than 0 offset is %d\n",offset);
				
				lseek(courseFD,-2 *sizeof(struct Course),SEEK_CUR);
				printf("\n After offset %d\n",offset);
				write(courseFD,&course,sizeof(struct Course));
			}
			else if(readBytes == 0)
			{
				printf("\nreadyBytes is zero %d\n",offset);
				lseek(courseFD,-1 *sizeof(struct Course),SEEK_CUR);
				printf("\n After offset %d\n",offset);
				ftruncate(courseFD, lseek(courseFD, 0, SEEK_CUR));
				break;
			}
			
		}
		printf("\nEnd of loop\n");
		lock.l_type = F_UNLCK;
		lockstatus = fcntl(courseFD, F_SETLK, &lock);
		if(lockstatus == -1)
		{
			perror("Error while releasing write lock on the course file!");
			close(courseFD);
			return false;
		}
		else
		printf("\nWrite Lock released\n");
	}
	else
	printf("\nData Not Found\n");
	
	close(courseFD);	
	writeBytes = write(sockfd, ADMIN_MOD_SUCCESS, strlen(ADMIN_MOD_SUCCESS));
	if(writeBytes == -1)
	{
		perror("Error while  ADMIN_MOD_SUCCESS");
		return false;
	}
	
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read

    	return true;	
}

#endif 
