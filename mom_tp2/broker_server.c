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

// Parsing data from configuration file

typedef struct broker_data {
	char broker_ip[20];
	char broker_port[10];
	char next_ip[20];
	char next_port[10];
	size_t broker_id;
	size_t broker_amount;
} broker_data_t;

bool parse_broker_data(broker_data_t* bd, char* argv[]){
	sscanf(argv[2], "%zu", &bd->broker_id);
	sscanf(argv[3], "%zu", &bd->broker_amount);
	
	FILE *fp = fopen (argv[1], "r");
	if(!fp) {
		printf("%d: Error opening configuration file %s: %d\n", getpid(), argv[1], errno);
		return false;
	}
	
	// Loop configuration file trying to find broker IP and port
	// and the next broker in ring IP and port
	size_t next_id = (bd->broker_id % bd->broker_amount) + 1;
	char read_ip[20], read_port[10];
	size_t read_id;
	bool read_mine = false, read_next = false;
	while(!(read_mine && read_next)){
		if(fscanf(fp, "%zu %s %s", &read_id, read_ip, read_port) < 3)
			continue;
		if(read_id == bd->broker_id) {
			sprintf(bd->broker_ip, "%s", read_ip);
			sprintf(bd->broker_port, "%s", read_port);
			read_mine = true;
		}
		else if(read_id == next_id) {
			sprintf(bd->next_ip, "%s", read_ip);
			sprintf(bd->next_port, "%s", read_port);
			read_next = true;
		}
	}
	
	// If here, I have finished scanning the file
	fclose(fp);
	if(!read_mine) {
		printf("%d: Error reading from configuration file %s: broker data not found!\n", getpid(), argv[1]);
		return false;
	}
	
	if(!read_next) {
		printf("%d: Error reading from configuration file %s: next data not found!\n", getpid(), argv[1]);
		return false;
	}
	
	return true;
}

// Handler for graceful quit
 
bool keep_looping = true;

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

bool launch_ring_master(ap_t* ap) {
	pid_t pid = fork();
	if(pid < 0) {
		printf("%d: Error launching DBMS: %d\n", getpid(), errno);
		return false;
	}
	if(pid == 0) {
		// If here, this is the ring master
		ap_exec(ap);
	}
	return true;
}

bool allocate_resources(broker_data_t bd, ap_t** ap_handler, ap_t** ap_rm, socket_t** s) {
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
	
	*ap_rm = ap_create("./ring_master");
	if(!*ap_handler){
		printf("%d: Error creating ring master parser:%d\n", getpid(), errno);
		return false;
	}
	
	// Create queues for handlers and senders
	create_temporal(QUEUE_HANDLER);
	create_temporal(QUEUE_SENDER);
	create_temporal(QUEUE_RM);
	int msqid_handler = msq_create(QUEUE_HANDLER);
	int msqid_sender = msq_create(QUEUE_SENDER);
	int msqid_rm = msq_create(QUEUE_RM);
	
	ap_set_int(ap_dbms, QUEUE_HANDLER, msqid_handler);
	ap_set_int(ap_dbms, QUEUE_SENDER, msqid_sender);
	ap_set_int(*ap_handler, QUEUE_HANDLER, msqid_handler);
	ap_set_int(*ap_handler, QUEUE_SENDER, msqid_sender);
	ap_set_int(*ap_handler, QUEUE_RM, msqid_rm);
	ap_set_int(*ap_rm, QUEUE_HANDLER, msqid_handler);
	ap_set_int(*ap_rm, QUEUE_RM, msqid_rm);
	ap_set_int(*ap_rm, BROKER_ID, bd.broker_id);
	ap_set_int(*ap_rm, BROKER_AMOUNT, bd.broker_amount);
	ap_set_string(*ap_rm, NEXT_IP, bd.next_ip);
	ap_set_string(*ap_rm, NEXT_PORT, bd.next_port);
	
	// Launch DBMS process
	launch_dbms(ap_dbms);
	ap_destroy(ap_dbms);	
	
	// Create broker side socket
	*s = socket_create(SOCK_PASSIVE);
	if(!*s){
		printf("%d: Error creating socket", getpid());
		return false; 
	}
	
	socket_bind(*s, bd.broker_ip, bd.broker_port);
	socket_listen(*s, 0);
	return true;
}

void release_resources(ap_t* ap_handler, ap_t* ap_rm, socket_t* s) {
	socket_destroy(s);
	
	int msqid_handler, msqid_sender, msqid_rm;
		
	ap_get_int(ap_handler, QUEUE_HANDLER, &msqid_handler);
	ap_get_int(ap_handler, QUEUE_SENDER, &msqid_sender);
	ap_get_int(ap_rm, QUEUE_RM, &msqid_rm);
	
	msq_destroy(msqid_handler);
	msq_destroy(msqid_sender);
	msq_destroy(msqid_rm);
	
	ap_destroy(ap_handler);
	ap_destroy(ap_rm);
	
	unlink(QUEUE_HANDLER);
	unlink(QUEUE_SENDER);
	unlink(QUEUE_RM);
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
	printf("./broker_server CONFIG_FILE BROKER_ID BROKERS_AMOUNT\n");
}

int main(int argc, char* argv[]){
	if(argc < 4) {
		printf("ERROR READING PARAMETERS\n");
		print_help();
		return -1;
	}
	
	broker_data_t bd = {0};
	if(!parse_broker_data(&bd, argv))
		return -1;
	
	// Set handler for graceful quit
	set_handler();
	ap_t* ap_handler = NULL;
	ap_t* ap_rm = NULL;
	socket_t* s = NULL;
	
	if(!allocate_resources(bd, &ap_handler, &ap_rm, &s))
		return -1;
	
	
	int children_amount = 1;
	keep_looping = true;
	
	if(launch_ring_master(ap_rm))
		children_amount++;
		
	printf("Broker server %zu of %zu is up!\n", bd.broker_id, bd.broker_amount);
	printf("Connected at %s port %s\n", bd.broker_ip, bd.broker_port);
	
	
	while(keep_looping){
		if(launch_handler(ap_handler, s))
			children_amount += 2; // A pair handler-sender, right?
	}
	
	printf("\nClosing broker server...\n");
	// Wait all children to finish
	for(int i = 0; i < children_amount; i++)
		wait(NULL);
	
	release_resources(ap_handler, ap_rm, s);
	printf("\nBroker server closed.\n");
	return 0;
}
