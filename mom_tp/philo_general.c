#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "mom.h"
#include "philo_general.h"


// Comunication with coordinator of dshm and dsem

/* The parent would read from parent_pipe[0] and write to 
 * child_pipe[1], and the child (the proxy to coordinators) 
 * would read from child_pipe[0] and write to parent_pipe[1]*/

int parent_pipe[2];
int child_pipe[2];

void write_coordinator_message(char* dresource, char* action, char* name, int value, char* dest){
	sprintf(dest, "%s %s %s %d\n", dresource, action, name, value);
}

int send_message_to_coord(mom_t* mom, char* m) {
	// Send message to coordinator
	if(write(child_pipe[1], m, strlen(m)) < 0) {
		printf("%d: Error on sending message %s to coordinator: %d\n", getpid(), m, errno);
		exit(-1);
	}
	// Await response from coordinator
	char response[COORD_MSG_SIZE] = {0};
	if(read(parent_pipe[0], response, COORD_MSG_SIZE) < 0) {
		printf("%d: Error on sending message %s to coordinator: bad reply!\n", getpid(), m);
		exit(-1);
	}
	//printf("%d: Sent %sGot %s (len %d)\n", getpid(), m, response, strlen(response));
	int r;
	sscanf(response, "Coord:%d", &r);
	return r;
}

void subscribe_to_coordinator(mom_t* mom, int priority){
	
	if(pipe(parent_pipe) || pipe(child_pipe)) {
        printf("%d: Error subscribing to coordinator: %d\n", getpid(), errno);
        exit(-1);
    }
	
	pid_t pid = fork();
    if (pid < 0) {
        printf("%d: Error forkig in subscribing to coordinator: %d\n", getpid(), errno);
        exit(1);
    }

    if (pid == 0) {
        // Proxy to coordinator
        close(child_pipe[1]);
        close(parent_pipe[0]);
        dup2(child_pipe[0], 0);
        dup2(parent_pipe[1], 1);
        char cmd_proxy[50] = {0};
        sprintf(cmd_proxy, "python %s %d", PROXY_FILE_PY, priority);
		system(cmd_proxy);
        close(child_pipe[0]);
        close(parent_pipe[1]);
		exit(0);
    }
    else {
        close(child_pipe[0]);
        close(parent_pipe[1]);
	}
}

void dshm_init(mom_t* mom, char* name, int value) {
	char m[COORD_MSG_SIZE] = {0};
	write_coordinator_message("SHM", "INIT", name, value, m);
	int r = send_message_to_coord(mom, m);
	if(r == 0) {
		printf("%d: Error initializing dshm %s with value %d\n", getpid(), name, value);
		exit(-1);		
	}
}

void dshm_read(mom_t* mom, char* name, int* value) {
	char m[COORD_MSG_SIZE] = {0};
	write_coordinator_message("SHM", "READ", name, 0, m);
	int r = send_message_to_coord(mom, m);
	*value = r;	
}

void dshm_write(mom_t* mom, char* name, int value) {
	char m[COORD_MSG_SIZE] = {0};
	write_coordinator_message("SHM", "WRITE", name, value, m);
	int r = send_message_to_coord(mom, m);
	if(r == 0) {
		printf("%d: Error writing dshm %s with value %d\n", getpid(), name, value);
		exit(-1);		
	}
}

void dsem_init(mom_t* mom, char* name, int value) {
	char m[COORD_MSG_SIZE] = {0};
	write_coordinator_message("SEM", "INIT", name, value, m);
	int r = send_message_to_coord(mom, m);
	if(r == 0) {
		printf("%d: Error initializing dsem %s with value %d\n", getpid(), name, value);
		exit(-1);		
	}
}

void dsem_signal(mom_t* mom, char* name) {
	char m[COORD_MSG_SIZE] = {0};
	write_coordinator_message("SEM", "SIGNAL", name, 0, m);
	int r = send_message_to_coord(mom, m);
	if(r == 0) {
		printf("%d: Error signaling dsem %s\n", getpid(), name);
		exit(-1);		
	}	
}

void dsem_wait(mom_t* mom, char* name) {
	char m[COORD_MSG_SIZE] = {0};
	write_coordinator_message("SEM", "WAIT", name, 0, m);
	int r = send_message_to_coord(mom, m);
	if(r == 0) {
		printf("%d: Error waiting dsem %s\n", getpid(), name);
		exit(-1);		
	}		
}

void dsem_destroy(mom_t* mom, char* name){
	char m[COORD_MSG_SIZE] = {0};
	write_coordinator_message("SEM", "DESTROY", name, 0, m);
	int r = send_message_to_coord(mom, m);
	if(r == 0) {
		printf("%d: Error destroying dsem %s\n", getpid(), name);
		exit(-1);		
	}		
}


void dshm_destroy(mom_t* mom, char* name){
	char m[COORD_MSG_SIZE] = {0};
	write_coordinator_message("SHM", "DESTROY", name, 0, m);
	int r = send_message_to_coord(mom, m);
	if(r == 0) {
		printf("%d: Error destroying dshm %s\n", getpid(), name);
		exit(-1);		
	}			
}
