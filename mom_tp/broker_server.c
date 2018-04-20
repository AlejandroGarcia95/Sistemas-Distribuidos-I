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

/* We all need to be mocked from time to time, 
 * lest we take ourselves too seriously. */
 
bool keep_looping = true;

void handler(int signum) {
  printf("Closing broker server...\n");
  keep_looping = false;
}

void set_handler() {
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = handler;
	sigaction(SIGINT, &sa, NULL);
}

/* Creates a temporal file with a given file_name. */
void create_temporal(char* file_name){
	if(mknod(file_name, S_IFREG|0666, 0) < 0) {
			printf("%d: Error creating temporal file %s: %d\n", getpid(), file_name, errno);
			exit(-1);
		}
}

bool allocate_resources(ap_t** ap_handler, socket_t** s) {
	*ap_handler = ap_create("./broker_handler");
	if(!*ap_handler){
		printf("%d: Error creating handler parser:%d\n", getpid(), errno);
		return false;
	}
	
	// Create broker side socket
	*s = socket_create(SOCK_PASSIVE);
	if(!*s){
		printf("%d: Error creating socket", getpid());
		return false; 
	}
	
	socket_bind(*s, SERVER_IP, SERVER_PORT);
	socket_listen(*s, 0);
	
	// Create queues for handlers and senders
	create_temporal(QUEUE_HANDLER);
	create_temporal(QUEUE_SENDER);
	int msqid_handler = msq_create(QUEUE_HANDLER);
	int msqid_sender = msq_create(QUEUE_SENDER);
	
	ap_set_int(*ap_handler, QUEUE_HANDLER, msqid_handler);
	ap_set_int(*ap_handler, QUEUE_SENDER, msqid_sender);
	
	return true;
}

void release_resources(ap_t* ap_handler, socket_t* s) {
	socket_destroy(s);
	
	int msqid_handler, msqid_sender;
		
	ap_get_int(ap_handler, QUEUE_HANDLER, &msqid_handler);
	ap_get_int(ap_handler, QUEUE_SENDER, &msqid_sender);
	
	msq_destroy(msqid_handler);
	msq_destroy(msqid_sender);
	
	ap_destroy(ap_handler);
	
	unlink(QUEUE_HANDLER);
	unlink(QUEUE_SENDER);
}

void launch_handler(ap_t* ap_handler, socket_t* s){
	socket_t* s2 = socket_accept(s);
	
	pid_t pid = fork();
	if(pid < 0) {
		printf("%d: Error launching handler: %d\n", getpid(), errno);
		socket_destroy(s2);
		return;
	}
	if(pid == 0) {
		// If here, this is the handler
		socket_destroy(s);
		int socket_fd = socket_get_fd(s2);
		ap_set_int(ap_handler, SOCKET_FD, socket_fd);
		ap_exec(ap_handler);
	}
	
	socket_destroy(s2);
}

int main(int argc, char* argv[]){
	// Set handler for graceful quit
	set_handler();
	ap_t* ap_handler = NULL;
	socket_t* s = NULL;
	
	if(!allocate_resources(&ap_handler, &s))
		return -1;
	
	keep_looping = true;
	printf("Broker server is up!\n");
	
	
	
	while(keep_looping){
		launch_handler(ap_handler, s);
	}
	
	release_resources(ap_handler, s);
	
	return 0;
}
