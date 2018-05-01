#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "mom.h"
#include "museum_general.h"


// Comunication with coordinator of dshm and dsem


void write_coordinator_message(char* dresource, char* action, char* name, int value, char* dest){
	sprintf(dest, "%d:%s %s %s %d", getpid(), dresource, action, name, value);
}

int send_message_to_coord(mom_t* mom, char* m) {
	// Send message
	if(!mom_publish(mom, COORD_TOPIC, (void*) m)) {
		printf("%d: Error on sending message %s to coordinator: wasn't reached!\n", getpid(), m);
		exit(-1);
	}
	// Await response;
	char response[COORD_MSG_SIZE];
	if(!mom_receive(mom, (void*) response)) {
		printf("%d: Error on sending message %s to coordinator: bad reply!\n", getpid(), m);
		exit(-1);
	}
	int r;
	sscanf(response, "Coord:%d", &r);
	return r;
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
