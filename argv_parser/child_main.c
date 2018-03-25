#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include "argv_parser.h"


int main(int argc, char* argv[]){
	printf("I am child %d and my argc is %d\n", getpid(), argc);
	// Create exec_parser
	ap_t* ap = ap_create_from_argv(argc, argv);
	if(!ap) {
		printf("Error creating exec_parser:%d\n", errno);
		exit(-1);
	}

	// Get some parameters
	int ppid; 
	ap_get_int(ap, "Parent PID", &ppid);
	printf("I am child %d and my parent is %d\n", getpid(), ppid);
	double a_parm;
	ap_get_double(ap, "Other parm", &a_parm);
	printf("I am child %d and the other param was %lf\n", getpid(), a_parm);
	
	ap_destroy(ap);

	exit(0);
}
