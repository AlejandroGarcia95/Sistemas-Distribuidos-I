#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mom_general.h"
#include "libs/msq.h"
#include "libs/socket.h"
#include "libs/argv_parser.h"

/* Creates a temporal file with a given file_name. */
void create_temporal(char* file_name){
	if(mknod(file_name, S_IFREG|0666, 0) < 0) {
			// In case they already exist, I ain't care
			if(errno == EEXIST)	return;

			printf("%d: Error creating temporal file %s: %d\n", getpid(), file_name, errno);
			exit(-1);
		}
}


bool allocate_resources(ap_t** ap_requester, ap_t** ap_responser, socket_t** s) {
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
	
	// Create queues
	create_temporal(QUEUE_REQUESTER);
	create_temporal(QUEUE_RESPONSER);
	int msqid_requester = msq_create(QUEUE_REQUESTER);
	int msqid_responser = msq_create(QUEUE_RESPONSER);
	
	ap_set_int(*ap_requester, QUEUE_REQUESTER, msqid_requester);
	ap_set_int(*ap_responser, QUEUE_RESPONSER, msqid_responser);
	
	// Create socket
	*s = socket_create(SOCK_ACTIVE);
	if(!*s){
		printf("%d: Error creating socket", getpid());
		exit(-1); 
	}
	
	socket_conect(*s, SERVER_IP, SERVER_PORT);
	printf("Connection established with broker\n");
	int socket_fd = socket_get_fd(*s);
	ap_set_int(*ap_requester, SOCKET_FD, socket_fd);
	ap_set_int(*ap_responser, SOCKET_FD, socket_fd);
	
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
	int msqid_requester, msqid_responser;
		
	ap_get_int(ap_requester, QUEUE_REQUESTER, &msqid_requester);
	ap_get_int(ap_responser, QUEUE_RESPONSER, &msqid_responser);
	
	msq_destroy(msqid_responser);
	msq_destroy(msqid_requester);
	
	ap_destroy(ap_responser);
	ap_destroy(ap_requester);
	
	unlink(QUEUE_REQUESTER);
	unlink(QUEUE_RESPONSER);
}

void ignore_sigint() {
	sigset_t sigset;
	
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigprocmask(SIG_SETMASK, &sigset, NULL);
}

int main(int argc, char* argv[]) {
	// Allocate resources
	ap_t* ap_requester = NULL;
	ap_t* ap_responser = NULL;
	socket_t* s = NULL;
	
	if(!allocate_resources(&ap_requester, &ap_responser, &s))
		return -1;
	
	// Launch all mom processes
	printf("%d: Launching mom daemon...\n", getpid());
	
	if(!launch_process(ap_requester, "mom requester"))
		return -1;
	
	if(!launch_process(ap_responser, "mom responser"))
		return -1;
	
	socket_destroy(s);
	
	ignore_sigint();
	// Release resources when finished
	wait(NULL);
	wait(NULL);
	
	release_resources(ap_requester, ap_responser);
	printf("\nMom daemon fully closed.\n");
	return 0;
}
