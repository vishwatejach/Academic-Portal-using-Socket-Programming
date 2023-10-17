#include "../Structures/course.h"
#ifndef STUDENT_RECORD
#define STUDENT_RECORD

struct Student
{
    int id;
    int active;		// 0 --> Deactive , 1 --> Activate
    char name[40];
    char gender;	// M --> Male, F --> Female, O --> Other
    int age;		// Age of stundet
    char email[40];
    char address[200];
    struct Course course; 
    // Login Credentials
    char login[30]; 	
    char password[30];	
};

#endif
