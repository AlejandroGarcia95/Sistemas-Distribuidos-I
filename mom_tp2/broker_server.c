#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
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
char* broker_ip = NULL;
char* broker_port = NULL;

void handler(int signum) {
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

bool launch_dbms(ap_t* ap) {
	pid_t pid = fork();
	if(pid < 0) {
		printf("%d: Error launching DBMS: %d\n", getpid(), errno);
		return false;
	}
	if(pid == 0) {
		// If here, this is the DBMS
		ap_exec(ap);
	}
	return true;
}


bool allocate_resources(ap_t** ap_handler, socket_t** s) {
	*ap_handler = ap_create("./broker_handler");
	if(!*ap_handler){
		printf("%d: Error creating handler parser:%d\n", getpid(), errno);
		return false;
	}
	
	ap_t* ap_dbms = ap_create("./dbms");
	if(!*ap_handler){
		printf("%d: Error creating dbms parser:%d\n", getpid(), errno);
		return false;
	}
	
	// Create queues for handlers and senders
	create_temporal(QUEUE_HANDLER);
	create_temporal(QUEUE_SENDER);
	int msqid_handler = msq_create(QUEUE_HANDLER);
	int msqid_sender = msq_create(QUEUE_SENDER);
	
	ap_set_int(ap_dbms, QUEUE_HANDLER, msqid_handler);
	ap_set_int(ap_dbms, QUEUE_SENDER, msqid_sender);
	ap_set_int(*ap_handler, QUEUE_HANDLER, msqid_handler);
	ap_set_int(*ap_handler, QUEUE_SENDER, msqid_sender);
	
	// Launch DBMS process
	launch_dbms(ap_dbms);
	ap_destroy(ap_dbms);
	
	// Create broker side socket
	*s = socket_create(SOCK_PASSIVE);
	if(!*s){
		printf("%d: Error creating socket", getpid());
		return false; 
	}
	
	socket_bind(*s, broker_ip, broker_port);
	socket_listen(*s, 0);
	
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

bool launch_handler(ap_t* ap_handler, socket_t* s){
	socket_t* s2 = socket_accept(s);
	
	if(!s2)	return false;
	
	pid_t pid = fork();
	if(pid < 0) {
		printf("%d: Error launching handler: %d\n", getpid(), errno);
		socket_destroy(s2);
		return false;
	}
	if(pid == 0) {
		// If here, this is the handler
		socket_destroy(s);
		int socket_fd = socket_get_fd(s2);
		ap_set_int(ap_handler, SOCKET_FD, socket_fd);
		ap_exec(ap_handler);
	}
	
	socket_destroy(s2);
	return true;
}


void print_help(){
	printf("USAGE:\n");
	printf("./broker_server BROKER_IP BROKER_PORT\n");
}

int main(int argc, char* argv[]){
	if(argc < 3) {
		printf("ERROR READING PARAMETERS\n");
		print_help();
		return -1;
	}
	
	broker_ip = argv[1];
	broker_port = argv[2];
	
	// Set handler for graceful quit
	set_handler();
	ap_t* ap_handler = NULL;
	socket_t* s = NULL;
	
	if(!allocate_resources(&ap_handler, &s))
		return -1;
	
	int children_amount = 1;
	keep_looping = true;
	printf("Broker server is up!\n");
	
	
	while(keep_looping){
		if(launch_handler(ap_handler, s))
			children_amount += 2; // A pair handler-sender, right?
	}
	
	printf("\nClosing broker server...\n");
	// Wait all children to finish
	for(int i = 0; i < children_amount; i++)
		wait(NULL);
	
	release_resources(ap_handler, s);
	printf("\nBroker server closed.\n");
	return 0;
}
