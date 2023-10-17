#ifndef COURSE_RECORD
#define COURSE_RECORD

struct Course
{
    int id;
    int facultyID;
    char name[100];
    char department[100];
    int totalSeats;			// Total seats available
    int credit;	
    int enrolledStudent;		// No. of student enrolled in this subject
};

#endif
