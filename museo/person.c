#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include "sem.h"
#include "msq.h"
#include "shm.h"
#include "general.h"
#include "argv_parser.h"

void inside_museum(ap_t* ap, int person_id){
	usleep(30000 + (rand() % 30000)); // Spend some time inside museum
	// Randomly choose door
	int door_id = rand() % DOOR_AMOUNT;
	// Retrieve that door queues
	char req_q_name[40], resp_q_name[40];
	sprintf(req_q_name, "request_queue_%d", door_id);
	sprintf(resp_q_name, "response_queue_%d", door_id);
	int req_q, resp_q;
	ap_get_int(ap, req_q_name, &req_q);
	ap_get_int(ap, resp_q_name, &resp_q);
	// Send request to exit
	message_t msg = {person_id, REQUEST_EXIT};
	
	printf("%d: Person %d requesting to exit through door %d\n", getpid(), person_id, door_id);
	msq_send(req_q, &msg, sizeof(message_t));
	
	// Receive response
	msq_rcv(resp_q, &msg, sizeof(msg), person_id);
}

int main(int argc, char* argv[]) {
	ap_t* ap = ap_create_from_argv(argc, argv);
	srand(getpid()); 

	if(!ap) {
		printf("Error creating argv_parser:%d\n", errno);
		exit(-1);
	}
	
	int person_id;
	ap_get_int(ap, "Person id", &person_id);
	
	// Randomly choose door
	int door_id = rand() % DOOR_AMOUNT;
	
	// Retrieve that door queues
	char req_q_name[40], resp_q_name[40];
	sprintf(req_q_name, "request_queue_%d", door_id);
	sprintf(resp_q_name, "response_queue_%d", door_id);
	int req_q, resp_q;
	ap_get_int(ap, req_q_name, &req_q);
	ap_get_int(ap, resp_q_name, &resp_q);
	
	// Send request to enter
	message_t msg = {person_id, REQUEST_ENTER};
	
	printf("%d: Person %d requesting to enter through door %d\n", getpid(), person_id, door_id);
	msq_send(req_q, &msg, sizeof(message_t));
	
	// Receive response
	msq_rcv(resp_q, &msg, sizeof(msg), person_id);
	if(msg.msg_type == ACCEPTED) {
		printf("%d: Person %d was accepted by door %d\n", getpid(), person_id, door_id);
		inside_museum(ap, person_id);		
	}
	else {
		printf("%d: Person %d was rejected by door %d\n", getpid(), person_id, door_id);
	}

	
	ap_destroy(ap);
	exit(0);
}
