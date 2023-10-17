#include "../Structures/course.h"
#ifndef FACULTY_RECORD
#define FACULTY_RECORD

struct Faculty
{
    int id;		
    char name[40];
    char department[30];
    char designation[30];
    char email[50];
    char address[200];
    struct Course course;
    // Login Credentials
    char login[30]; 	
    char password[30];	
};

#endif
