#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include "libs/msq.h"
#include "libs/argv_parser.h"

bool allocate_resources(ap_t** ap_requester, ap_t** ap_responser) {
	*ap_requester = ap_create("./mom_requester");
	if(!*ap_requester) {
		printf("%d: Error creating mom_requester parser:%d\n", getpid(), errno);
		return false;
	}
	
	*ap_responser = ap_create("./mom_responser");
	if(!*ap_responser) {
		printf("%d: Error creating mom_responser parser:%d\n", getpid(), errno);
		ap_destroy(*ap_requester);
		return false;
	}
	
	int msqid = msq_create("mom.h");
	int msq_sockets = msq_create("mom_responser.c"); // To mock socket !!
	
	ap_set_int(*ap_requester, "msq", msqid);
	ap_set_int(*ap_responser, "msq", msqid);
	ap_set_int(*ap_requester, "socket", msq_sockets);
	ap_set_int(*ap_responser, "socket", msq_sockets);
	
	return true;	
}

bool launch_process(ap_t* ap, char* process_name) {
	pid_t pid = fork();
	if(pid < 0) {
		printf("%d: Error launching %s: %d\n", getpid(), process_name, errno);
		return false;
	}
	if(pid == 0) {
		ap_exec(ap);
	}
	return true;
}

void release_resources(ap_t* ap_requester, ap_t* ap_responser) {
	int msqid, msq_sockets;
	ap_get_int(ap_requester, "socket", &msqid);
	ap_get_int(ap_requester, "msq", &msq_sockets);
	
	msq_destroy(msqid);
	msq_destroy(msq_sockets);
	
	ap_destroy(ap_responser);
	ap_destroy(ap_requester);
}

int main(int argc, char* argv[]) {
	// Allocate resources
	ap_t* ap_requester = NULL;
	ap_t* ap_responser = NULL;
	
	if(!allocate_resources(&ap_requester, &ap_responser))
		return -1;
	
	// Launch all mom processes
	printf("Hello! I am the daemon!\n");
	
	if(!launch_process(ap_requester, "mom_requester"))
		return -1;
	
	if(!launch_process(ap_responser, "mom_responser"))
		return -1;
	
	// Release resources when finished
	wait(NULL);
	wait(NULL);
	
	release_resources(ap_requester, ap_responser);
	return 0;
}
