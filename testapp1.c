#include "uapi_mm.h"

typedef struct{
	char name[32];
	uint32_t id;
} emp;

typedef struct _student{
	char name[32];
	uint32_t rollno;
	uint32_t marks1;
	uint32_t marks2;
	uint32_t marks3;
	struct _student *next;
} student;

int main(int argc, char **argv){

	mm_init();

	MM_REG_STRUCT(emp);
	MM_REG_STRUCT(student);
	mm_print_registered_page_families();

	return 0;
}