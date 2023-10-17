#include <stdbool.h>
#include "./constant.h"
#include "./default.h"
#ifndef ADMIN_FUNCTIONS
#define ADMIN_FUNCITONS

bool adminHandler(int sockfd);
int addStudent(int sockfd);
int addFaculty(int sockfd);
bool changeActive(int sockfd,int command);
bool modifyStudent(int sockfd);
bool modifyFaculty(int sockfd);

bool adminHandler(int sockfd)
{
	int returnLog;
	if((returnLog = loginHandler(0,sockfd)) != -1)
	{
		if(returnLog == -99)
		{
			// If Username or password didnt match
			printf("\n Data not found !! -99\n");
			write(sockfd, LOGOUT, strlen(LOGOUT));
			return false;
				
		}
		printf("\nInto admin.h\n");
		int readBytes, writeBytes;			// Number of bytes written to / read from the socket 
		char readBuffer[1024], writeBuffer[1024];	// Buffer for reading from / writing to the client 
		bzero(writeBuffer, sizeof(writeBuffer));
		strcpy(writeBuffer,"Welcome Admin!!");
		while(1)
		{
			strcat(writeBuffer, "\n");
			strcat(writeBuffer, ADMIN_MENU);
			writeBytes = write(sockfd, writeBuffer, strlen(writeBuffer));
			if (writeBytes == -1)
			{
				perror("Error while writing ADMIN_MENU to client!");
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
			
			switch(choice)
			{
				case 1:
					addStudent(sockfd);
					break;
				case 2:
					get_student_details(sockfd,-1);
					break;
				case 3:
					addFaculty(sockfd);
					break;
				case 4:
					get_faculty_details(sockfd,-1);
					break;
				case 5:
					changeActive(sockfd,1);
					break;
				case 6:
					changeActive(sockfd,0);
					break;
				case 7:
					modifyStudent(sockfd);
					break;
				case 8:
					modifyFaculty(sockfd);
					break;
				default:
					writeBytes = write(sockfd, LOGOUT, strlen(LOGOUT));
                			return false;
			}
		}
	}
	else
	{
		// ADMIN LOGIN FAILED
        	return false;
	}
	return true;
}

// ---------------------------------------------------------------------------------------------
// ---------------------------- ADD Student ----------------------------------------------------
// ---------------------------------------------------------------------------------------------

int addStudent(int sockfd)
{
	int readBytes, writeBytes;
	char readBuffer[1024], writeBuffer[1024];
	
	struct Student newStudent, oldStudent;
	newStudent.active = 1;
	int studentFD = open(STUDENT_FILE, O_RDONLY);
	if(studentFD == -1 && errno == ENOENT)		// ENOENT : represents the error code for "No such file or directory"
	{
		// student file was never created
		newStudent.id = 0;
	}
	else if(studentFD == -1)
	{
		perror("Error while openeing student file");
		return -1;
	}
	else
	{
		int offset = lseek(studentFD,-sizeof(struct Student), SEEK_END);
		if(offset == -1)
		{
			perror("Error seeking to last student record");
			return -1;
		}
		
		struct flock lock = {F_RDLCK,SEEK_SET,offset,sizeof(struct Student),getpid()};
		int lockstatus = fcntl(studentFD, F_SETLKW, &lock);
		if(lockstatus == -1)
		{
			perror("Error while obtaining read lock");
			return -1;
		}
		
		readBytes = read(studentFD,&oldStudent,sizeof(struct Student));
		if(readBytes == -1)
		{
			perror("Error while reading student record from file");
			return -1;
		}
		
		lock.l_type = F_UNLCK;
		lockstatus = fcntl(studentFD,F_SETLK,&lock);
		if(lockstatus == -1)
		{
			perror("Error while releasing read lock");
			return -1;
		}
		
		close(studentFD);
		
		newStudent.id = oldStudent.id + 1;
	}
	
	//-----------------------------------------------------------------
	//---------------------------------- NAME -------------------------
	//-----------------------------------------------------------------
	bzero(writeBuffer, sizeof(writeBuffer));
	strcpy(writeBuffer, GET_NAME);
	writeBytes = write(sockfd, writeBuffer, sizeof(writeBuffer));
	if (writeBytes == -1)
	{
		perror("Error writing ADMIN_ADD_STUDENT_NAME message to client!");
		return -1;
	}
	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
    	if (readBytes == -1)
    	{
		perror("Error reading customer name response from client!");
		return -1;
	}
	
	strcpy(newStudent.name, readBuffer);
	printf("\nStudent name: %s\n",newStudent.name);
	
	//-----------------------------------------------------------------
	//--------------------------------GENDER---------------------------
	//-----------------------------------------------------------------
	writeBytes = write(sockfd, GET_GENDER, sizeof(GET_GENDER));
	if (writeBytes == -1)
	{
		perror("Error writing ADMIN_ADD_STUDENT_NAME message to client!");
		return -1;
	}
	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
	if (readBytes == -1)
	{
		perror("Error reading customer gender response from client!");
		return -1;
	}

	if (readBuffer[0] == 'M' || readBuffer[0] == 'F' || readBuffer[0] == 'O')
		newStudent.gender = readBuffer[0];
	else
	{
		writeBytes = write(sockfd, ADMIN_ADD_WRONG_GENDER, strlen(ADMIN_ADD_WRONG_GENDER));
		readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read
		return -1;
	}
	
	//-----------------------------------------------------------------
	//---------------------------------- AGE---------------------------
	//-----------------------------------------------------------------
	bzero(writeBuffer, sizeof(writeBuffer));
	writeBytes = write(sockfd, GET_AGE, strlen(GET_AGE));
	if (writeBytes == -1)
	{
		perror("Error writing GET_AGE message to client!");
		return -1;
	}

	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
	if (readBytes == -1)
	{
		perror("Error reading customer age response from client!");
		return -1;
	}

	int studentAge = atoi(readBuffer);
	if (studentAge == 0)
	{
		// Either student has sent age as 0 (which is invalid) or has entered a non-numeric string
		writeBytes = write(sockfd, ERRON_INPUT_FOR_NUMBER, strlen(ERRON_INPUT_FOR_NUMBER));
		if (writeBytes == -1)
		{
		    perror("Error while writing ERRON_INPUT_FOR_NUMBER message to client!");
		    return -1;
		}
		bzero(readBuffer, sizeof(readBuffer));
		readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read
		return -1;
	}
	newStudent.age = studentAge;
	
	//-----------------------------------------------------------------
	//---------------------------------- EMAIL-------------------------
	//-----------------------------------------------------------------
	bzero(writeBuffer,sizeof(writeBuffer));
	writeBytes = write(sockfd, GET_EMAIL, strlen(GET_EMAIL));
	if (writeBytes == -1)
	{
		perror("Error writing GET_EMAIL message to client!");
		return -1;
	}

	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
	if (readBytes == -1)
	{
		perror("Error reading customer age response from client!");
		return -1;
	}
	strcpy(newStudent.email, readBuffer);
	printf("\nStudent Email: %s\n",newStudent.email);
	
	//-----------------------------------------------------------------
	//----------------------------------ADDRESS------------------------
	//-----------------------------------------------------------------
	bzero(writeBuffer,sizeof(writeBuffer));
	writeBytes = write(sockfd, GET_ADDRESS, strlen(GET_ADDRESS));
	if (writeBytes == -1)
	{
		perror("Error writing GET_ADDRESS message to client!");
		return -1;
	}

	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
	if (readBytes == -1)
	{
		perror("Error reading customer age response from client!");
		return -1;
	}
	strcpy(newStudent.address, readBuffer);
	printf("\nStudent Address: %s\n",newStudent.address);	
	
	//-----------------------------------------------------------------
	//---------------------------------- LOGIN-------------------------
	//-----------------------------------------------------------------
	bzero(writeBuffer, sizeof(writeBuffer));
	strcpy(newStudent.login, "MT2023");
	sprintf(writeBuffer,"%d",newStudent.id);
	strcat(newStudent.login, writeBuffer);
	
	//-----------------------------------------------------------------
	//-------------------------------PASSWORD -------------------------
	//-----------------------------------------------------------------
	char hashedPassword[1000];
	strcpy(hashedPassword, crypt(AUTOGEN_PASSWORD, "MK"));
	strcpy(newStudent.password, hashedPassword);
	
	studentFD = open(STUDENT_FILE, O_CREAT | O_APPEND | O_WRONLY, 0666);
	if(studentFD == -1)
	{
		perror("Error while creating/opening student file");
		return -1;
	}
	
	writeBytes = write(studentFD, &newStudent, sizeof(newStudent));
	if (writeBytes == -1)
	{
		perror("Error while writing Student record to file!");
		return -1;
	}
	
	close(studentFD);
	
	bzero(writeBuffer, sizeof(writeBuffer));
	sprintf(writeBuffer, "%sMT2023%d\n%s%s",ADMIN_ADD_AUTOGEN_LOGIN,newStudent.id, ADMIN_ADD_AUTOGEN_PASSWORD, AUTOGEN_PASSWORD);
	strcat(writeBuffer, "^");
	writeBytes = write(sockfd, writeBuffer, strlen(writeBuffer));
	if (writeBytes == -1)
	{
		perror("Error sending student loginID and password to the client!");
		return -1;
	}

	readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read

	return newStudent.id;
}

// ---------------------------------------------------------------------------------------------
// ---------------------------- ADD Faculty ----------------------------------------------------
// ---------------------------------------------------------------------------------------------

int addFaculty(int sockfd)
{
	int readBytes, writeBytes;
	char readBuffer[1024], writeBuffer[1024];
	struct Faculty newFaculty, oldFaculty;
	
	int facultyFD = open(FACULTY_FILE, O_RDONLY);
	if(facultyFD == -1 && errno == ENOENT)		// ENOENT : represents the error code for "No such file or directory"
	{
		// faculty file was never created
		newFaculty.id = 0;
	}
	else if(facultyFD == -1)
	{
		perror("Error while openeing faculty file");
		return -1;
	}
	else
	{
		//lseek(facultyFD,0,SEEK_SET);
		int offset = lseek(facultyFD,-sizeof(struct Faculty), SEEK_END);
		if(offset == -1)
		{
			perror("Error seeking to last faculty record");
			return -1;
		}
		
		struct flock lock = {F_RDLCK,SEEK_SET,offset,sizeof(struct Faculty),getpid()};
		int lockstatus = fcntl(facultyFD, F_SETLKW, &lock);
		if(lockstatus == -1)
		{
			perror("Error while obtaining read lock");
			return -1;
		}
		
		readBytes = read(facultyFD,&oldFaculty,sizeof(struct Faculty));
		if(readBytes == -1)
		{
			perror("Error while reading faculty record from file");
			return -1;
		}
		
		lock.l_type = F_UNLCK;
		lockstatus = fcntl(facultyFD,F_SETLK,&lock);
		if(lockstatus == -1)
		{
			perror("Error while releasing read lock");
			return -1;
		}
		
		close(facultyFD);
		
		newFaculty.id = oldFaculty.id + 1;
	}
	//-----------------------------------------------------------------
	//---------------------------------- NAME -------------------------
	//-----------------------------------------------------------------
	bzero(writeBuffer, sizeof(writeBuffer));
	strcpy(writeBuffer, GET_NAME);
	writeBytes = write(sockfd, writeBuffer, sizeof(writeBuffer));
	if (writeBytes == -1)
	{
		perror("Error writing ADMIN_ADD_Faculty_NAME message to client!");
		return false;
	}
	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
    	if (readBytes == -1)
    	{
		perror("Error reading customer name response from client!");
		return false;
	}
	
	strcpy(newFaculty.name, readBuffer);
	printf("\nFaculty name: %s\n",newFaculty.name);
	
	//-----------------------------------------------------------------
	//------------------------------DEPARMENT -------------------------
	//-----------------------------------------------------------------
	bzero(writeBuffer, sizeof(writeBuffer));
	strcpy(writeBuffer, GET_DEPARMENT);
	writeBytes = write(sockfd, writeBuffer, sizeof(writeBuffer));
	if (writeBytes == -1)
	{
		perror("Error writing ADMIN_ADD_Faculty_NAME message to client!");
		return false;
	}
	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
    	if (readBytes == -1)
    	{
		perror("Error reading customer name response from client!");
		return false;
	}
	
	strcpy(newFaculty.department, readBuffer);
	printf("\nFaculty deparment: %s\n",newFaculty.department);
	
	//-----------------------------------------------------------------
	//----------------------------DESIGNATION -------------------------
	//-----------------------------------------------------------------
	bzero(writeBuffer, sizeof(writeBuffer));
	strcpy(writeBuffer, GET_DESIGNATION);
	writeBytes = write(sockfd, writeBuffer, sizeof(writeBuffer));
	if (writeBytes == -1)
	{
		perror("Error writing ADMIN_ADD_Faculty_NAME message to client!");
		return false;
	}
	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
    	if (readBytes == -1)
    	{
		perror("Error reading customer name response from client!");
		return false;
	}
	
	strcpy(newFaculty.designation, readBuffer);
	printf("\nFaculty designation: %s\n",newFaculty.designation);

	//-----------------------------------------------------------------
	//---------------------------------- EMAIL ------------------------
	//-----------------------------------------------------------------
	bzero(writeBuffer,sizeof(writeBuffer));
	writeBytes = write(sockfd, GET_EMAIL, strlen(GET_EMAIL));
	if (writeBytes == -1)
	{
		perror("Error writing GET_EMAIL message to client!");
		return false;
	}

	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
	if (readBytes == -1)
	{
		perror("Error reading customer age response from client!");
		return false;
	}
	strcpy(newFaculty.email, readBuffer);
	printf("\nFaculty Email: %s\n",newFaculty.email);
	
	//-----------------------------------------------------------------
	//---------------------------------- ADDRESS ----------------------
	//-----------------------------------------------------------------
	bzero(writeBuffer,sizeof(writeBuffer));
	writeBytes = write(sockfd, GET_ADDRESS, strlen(GET_ADDRESS));
	if (writeBytes == -1)
	{
		perror("Error writing GET_ADDRESS message to client!");
		return false;
	}

	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
	if (readBytes == -1)
	{
		perror("Error reading customer age response from client!");
		return false;
	}
	strcpy(newFaculty.address, readBuffer);
	printf("\nFaculty Address: %s\n",newFaculty.address);	
	
	//-----------------------------------------------------------------
	//---------------------------------- LOGIN ID ---------------------
	//-----------------------------------------------------------------
	bzero(writeBuffer, sizeof(writeBuffer));
	strcpy(newFaculty.login, "faculty");
	sprintf(writeBuffer,"%d",newFaculty.id);
	strcat(newFaculty.login, writeBuffer);
	
	//-----------------------------------------------------------------
	//---------------------------------- PASSWORD ---------------------
	//-----------------------------------------------------------------
	char hashedPassword[1000];
	strcpy(hashedPassword, crypt(AUTOGEN_PASSWORD, "MK"));
	strcpy(newFaculty.password, hashedPassword);
	
	facultyFD = open(FACULTY_FILE, O_CREAT | O_APPEND | O_WRONLY, 0666);
	if(facultyFD == -1)
	{
		perror("Error while creating/opening faculty file");
		return false;
	}
	
	writeBytes = write(facultyFD, &newFaculty, sizeof(newFaculty));
	if (writeBytes == -1)
	{
		perror("Error while writing Faculty record to file!");
		return false;
	}
	
	close(facultyFD);
	
	bzero(writeBuffer, sizeof(writeBuffer));
	sprintf(writeBuffer, "%sfaculty%d\n%s%s",ADMIN_ADD_AUTOGEN_LOGIN,newFaculty.id, ADMIN_ADD_AUTOGEN_PASSWORD, AUTOGEN_PASSWORD);
	strcat(writeBuffer, "^");
	writeBytes = write(sockfd, writeBuffer, strlen(writeBuffer));
	if (writeBytes == -1)
	{
		perror("Error sending faculty loginID and password to the client!");
		return false;
	}

	readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read

	return newFaculty.id;
}

// ---------------------------------------------------------------------------------------------
// ---------------------------- MODIFY STUDENT -------------------------------------------------
// ---------------------------------------------------------------------------------------------
bool modifyStudent(int sockfd)
{
	int readBytes, writeBytes;
	char readBuffer[1024], writeBuffer[1024];
	
	struct Student student;
	int studentID;
	
	off_t offset;
	int lockstatus;
	
	//-----------------------------------------------------------------
	//---------------------------------- ID ---------------------------
	//-----------------------------------------------------------------
	writeBytes = write(sockfd, ADMIN_MOD_STUDENT_ID, strlen(ADMIN_MOD_STUDENT_ID));
	if (writeBytes == -1)
	{
		perror("Error while writing ADMIN_MOD_CUSTOMER_ID message to client!");
		return false;
	}
	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
	if (readBytes == -1)
	{
		perror("Error while reading student ID from client!");
		return false;
	}
	studentID = atoi(readBuffer);
	
	int studentFD = open(STUDENT_FILE, O_RDONLY);
	if(studentFD == -1)
	{
		// student file doesnt exist
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
	lockstatus = fcntl(studentFD, F_SETLK, &lock);
	if(lockstatus == -1)
	{
		perror("Error while unlocking read lock on the student file!");
        	return false;
	}
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
	close(studentFD);
	
	
	//-----------------------------------------------------------------
	//-------------------------------- MENU ---------------------------
	//-----------------------------------------------------------------
	writeBytes = write(sockfd, ADMIN_MOD_STUDENT_MENU, strlen(ADMIN_MOD_STUDENT_MENU));
	if (writeBytes == -1)
	{
		perror("Error while writing ADMIN_MOD_STUDENT_MENU message to client!");
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
			    perror("Error while getting response for students's new name from client!");
			    return false;
			}
			strcpy(student.name, readBuffer);
			break;
		case 2:
			writeBytes = write(sockfd, ADMIN_MOD_NEW_AGE, strlen(ADMIN_MOD_NEW_AGE));
			if(writeBytes == -1)
			{
				perror("Error while writing ADMIN_MOD_NEW_AGE message to client!");
            			return false;
			}
			bzero(readBuffer, sizeof(readBuffer));
			readBytes = read(sockfd, &readBuffer, sizeof(readBuffer));
			if (readBytes == -1)
			{
			    perror("Error while getting response for students's new age from client!");
			    return false;
			}
			
			int studentAge = atoi(readBuffer);
			if (studentAge == 0)
			{
				// Either student has sent age as 0 (which is invalid) or has entered a non-numeric string
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
			student.age = studentAge;
			break;
		case 3:
			writeBytes = write(sockfd, ADMIN_MOD_NEW_GENDER, strlen(ADMIN_MOD_NEW_GENDER));
			if(writeBytes == -1)
			{
				perror("Error while writing ADMIN_MOD_NEW_GENDER message to client!");
            			return false;
			}
			bzero(readBuffer, sizeof(readBuffer));
			readBytes = read(sockfd, &readBuffer, sizeof(readBuffer));
			if (readBytes == -1)
			{
			    perror("Error while getting response for students's new gender from client!");
			    return false;
			}
			
			if (readBuffer[0] == 'M' || readBuffer[0] == 'F' || readBuffer[0] == 'O')
				student.gender = readBuffer[0];
			else
			{
				writeBytes = write(sockfd, ADMIN_ADD_WRONG_GENDER, strlen(ADMIN_ADD_WRONG_GENDER));
				readBytes = read(sockfd, readBuffer, sizeof(readBuffer)); // Dummy read
				return false;
			}
			break;
		case 4:
			writeBytes = write(sockfd, ADMIN_MOD_NEW_EMAIL, strlen(ADMIN_MOD_NEW_EMAIL));
			if(writeBytes == -1)
			{
				perror("Error while writing ADMIN_MOD_NEW_EMAIL message to client!");
            			return false;
			}
			bzero(readBuffer, sizeof(readBuffer));
			readBytes = read(sockfd, &readBuffer, sizeof(readBuffer));
			if (readBytes == -1)
			{
			    perror("Error while getting response for students's new email from client!");
			    return false;
			}
			strcpy(student.email, readBuffer);
			break;
		case 5:
			writeBytes = write(sockfd, ADMIN_MOD_NEW_ADDRESS, strlen(ADMIN_MOD_NEW_ADDRESS));
			if(writeBytes == -1)
			{
				perror("Error while writing ADMIN_MOD_NEW_ADDRESS message to client!");
            			return false;
			}
			bzero(readBuffer, sizeof(readBuffer));
			readBytes = read(sockfd, &readBuffer, sizeof(readBuffer));
			if (readBytes == -1)
			{
			    perror("Error while getting response for students's new address from client!");
			    return false;
			}
			strcpy(student.address, readBuffer);
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
		perror("Error while obtaining write lock on student record 2");
		return false;
	}else
	printf("\nUnlocked !!\n");
	
	writeBytes = write(studentFD, &student, sizeof(struct Student));
	if(writeBytes == -1)
	{
		perror("Error while writing update to student file");
		return false;
	}
	
	lock.l_type = F_UNLCK;
	lockstatus = fcntl(studentFD, F_SETLK, &lock);
	if(lockstatus == -1)
	{
		perror("Error while releasing write lock on student record 2");
		return false;
	}
	
	close(studentFD);
	
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
// ---------------------------- MODIFY FACULTY -------------------------------------------------
// ---------------------------------------------------------------------------------------------
bool modifyFaculty(int sockfd)
{
	int readBytes, writeBytes;
	char readBuffer[1024], writeBuffer[1024];
	
	struct Faculty faculty;
	int facultyID;
	
	off_t offset;
	int lockstatus;
	
	//-----------------------------------------------------------------
	//---------------------------------- ID ---------------------------
	//-----------------------------------------------------------------
	writeBytes = write(sockfd, ADMIN_MOD_FACULTY_ID, strlen(ADMIN_MOD_FACULTY_ID));
	if (writeBytes == -1)
	{
		perror("Error while writing ADMIN_MOD_FACULTY_ID message to client!");
		return false;
	}
	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
	if (readBytes == -1)
	{
		perror("Error while reading faculty ID from client!");
		return false;
	}
	facultyID = atoi(readBuffer);
	
	int facultyFD = open(FACULTY_FILE, O_RDONLY);
	if(facultyFD == -1)
	{
		// Faculty file doesnt exist
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
	lockstatus = fcntl(facultyFD, F_SETLK, &lock);
	if(lockstatus == -1)
	{
		perror("Error while releasing read lock on the faculty file!");
        	return false;
	}
	if (readBytes == -1)
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
	close(facultyFD);
	
	
	//-----------------------------------------------------------------
	//-------------------------------- MENU ---------------------------
	//-----------------------------------------------------------------
	writeBytes = write(sockfd, ADMIN_MOD_FACULTY_MENU, strlen(ADMIN_MOD_FACULTY_MENU));
	if (writeBytes == -1)
	{
		perror("Error while writing ADMIN_MOD_FACULTY_MENU message to client!");
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
				perror("Error while writing ERRON_INPUT_FOR_NUMBER message to client!");
            			return false;
			}
			bzero(readBuffer, sizeof(readBuffer));
			readBytes = read(sockfd, &readBuffer, sizeof(readBuffer));
			if (readBytes == -1)
			{
			    perror("Error while getting response for facultys's new name from client!");
			    return false;
			}
			strcpy(faculty.name, readBuffer);
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
			    perror("Error while getting response for facultys's new department from client!");
			    return false;
			}
			strcpy(faculty.department, readBuffer);
			break;
		case 3:
			writeBytes = write(sockfd, ADMIN_MOD_NEW_DESIGNATION, strlen(ADMIN_MOD_NEW_DESIGNATION));
			if(writeBytes == -1)
			{
				perror("Error while writing ADMIN_MOD_NEW_DESIGNATION message to client!");
            			return false;
			}
			bzero(readBuffer, sizeof(readBuffer));
			readBytes = read(sockfd, &readBuffer, sizeof(readBuffer));
			if (readBytes == -1)
			{
			    perror("Error while getting response for facultys's new designation from client!");
			    return false;
			}
			strcpy(faculty.designation, readBuffer);
			break;
		case 4:
			writeBytes = write(sockfd, ADMIN_MOD_NEW_EMAIL, strlen(ADMIN_MOD_NEW_EMAIL));
			if(writeBytes == -1)
			{
				perror("Error while writing ADMIN_MOD_NEW_EMAIL message to client!");
            			return false;
			}
			bzero(readBuffer, sizeof(readBuffer));
			readBytes = read(sockfd, &readBuffer, sizeof(readBuffer));
			if (readBytes == -1)
			{
			    perror("Error while getting response for facultys's new email from client!");
			    return false;
			}
			strcpy(faculty.email, readBuffer);
			break;
		case 5:
			writeBytes = write(sockfd, ADMIN_MOD_NEW_ADDRESS, strlen(ADMIN_MOD_NEW_ADDRESS));
			if(writeBytes == -1)
			{
				perror("Error while writing ADMIN_MOD_NEW_ADDRESS message to client!");
            			return false;
			}
			bzero(readBuffer, sizeof(readBuffer));
			readBytes = read(sockfd, &readBuffer, sizeof(readBuffer));
			if (readBytes == -1)
			{
			    perror("Error while getting response for facultys's new address from client!");
			    return false;
			}
			strcpy(faculty.address, readBuffer);
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
	if(writeBytes == -1)
	{
		perror("Error while writing update to faculty file");
		return false;
	}
	
	lock.l_type = F_UNLCK;
	lockstatus = fcntl(facultyFD, F_SETLK, &lock);
	if(lockstatus == -1)
	{
		perror("Error while releasing write lock on faculty record");
		return false;
	}
	
	close(facultyFD);
	
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
// ---------------------------- CHANGE ACTIVE STATE --------------------------------------------
// ---------------------------------------------------------------------------------------------
bool changeActive(int sockfd,int command)
{
	int readBytes, writeBytes;
	char readBuffer[1024], writeBuffer[1024];
	
	struct Student student;
	int studentID;
	
	off_t offset;
	int lockstatus;
	
	//-----------------------------------------------------------------
	//---------------------------------- ID ---------------------------
	//-----------------------------------------------------------------
	writeBytes = write(sockfd, ADMIN_MOD_STUDENT_ID, strlen(ADMIN_MOD_STUDENT_ID));
	if (writeBytes == -1)
	{
		perror("Error while writing ADMIN_MOD_CUSTOMER_ID message to client!");
		return false;
	}
	bzero(readBuffer, sizeof(readBuffer));
	readBytes = read(sockfd, readBuffer, sizeof(readBuffer));
	if (readBytes == -1)
	{
		perror("Error while reading student ID from client!");
		return false;
	}
	studentID = atoi(readBuffer);
	
	int studentFD = open(STUDENT_FILE, O_RDONLY);
	if(studentFD == -1)
	{
		// student file doesnt exist
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
	lockstatus = fcntl(studentFD, F_SETLK, &lock);
	if(lockstatus == -1)
	{
		perror("Error while releasing read lock on the student file!");
        	return false;
	}
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
	close(studentFD);
	
	student.active = command;
	
	studentFD = open(STUDENT_FILE, O_WRONLY);
	if(studentFD == -1)
	{
		perror("Error while opening student file");
        	return false;
	}
	lseek(studentFD, 0, SEEK_SET);
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
		perror("Error while obtaining write lock on student record to change Active state");
		return false;
	}
	
	writeBytes = write(studentFD, &student, sizeof(struct Student));
	if(writeBytes == -1)
	{
		perror("Error while writing update to student file");
		return false;
	}
	
	lock.l_type = F_UNLCK;
	lockstatus = fcntl(studentFD, F_SETLK, &lock);
	if(lockstatus == -1)
	{
		perror("Error while releasing write lock on student record to change Active state");
		return false;
	}
	
	close(studentFD);
	
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
