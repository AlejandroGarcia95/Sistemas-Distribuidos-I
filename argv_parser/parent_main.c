#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include "argv_parser.h"

#define CHILDREN_AMOUNT 2

int main(int argc, char* argv[]){
	// Create exec_parser
	ap_t* ap = ap_create("./child_main");
	if(!ap) {
		printf("Error creating exec_parser:%d\n", errno);
		return -1;
	}
	
	// Add some parameters
	ap_set_int(ap, "Parent PID", getpid());
	ap_set_double(ap, "Other parm", -4.2);
	
	// Launch children
	int i;
	for(i = 0; i < CHILDREN_AMOUNT; i++) {
		pid_t pid = fork();
		if(pid < 0) {
			printf("%d: Error launching child: %d\n", getpid(), errno);
			exit(-1);
		}
		if(pid == 0) {
			ap_exec(ap);
		}
	}
	
	// Wait children
	for(i = 0; i < CHILDREN_AMOUNT; i++)
		wait(NULL);
		
	ap_destroy(ap);

	return 0;
}
