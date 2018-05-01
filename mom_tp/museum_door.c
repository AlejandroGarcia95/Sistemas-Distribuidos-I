#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>

#include "mom.h"
#include "museum_general.h"
#include "libs/argv_parser.h"


int main(){
	// Child loop
	mom_t* mom = mom_create();
	
	if(!mom)	return 0;
	
	char my_topic[15] = {0};
	sprintf(my_topic, "Museum/%d", getpid());
	mom_subscribe(mom, my_topic);
	
	printf("Child ready\n");
	for(int i = 0; i < 6; i++) {
		int child_value = 3 * i + 2;
		dshm_write(mom, "child_shm", child_value);
		dsem_signal(mom, "parent_can_read");
		dsem_wait(mom, "child_can_read");
		int parent_value;
		dshm_read(mom, "parent_shm", &parent_value);
		printf("Door-Turn %d: (Parent: %d, Child:%d)\n", i + 1, parent_value, child_value);
		dsem_signal(mom, "child_finished");
		dsem_wait(mom, "parent_finished");
	}
	
	mom_destroy(mom);
	return 0;
}

/*
int main(int argc, char* argv[]) {
	srand(time(NULL)); 
	ap_t* ap = ap_create_from_argv(argc, argv);
	if(!ap) {
		printf("Error creating argv_parser:%d\n", errno);
		exit(-1);
	}
	
	int req_q, resp_q, semid, shmid, door_id;
	ap_get_int(ap, "request_queue", &req_q);
	ap_get_int(ap, "response_queue", &resp_q);
	ap_get_int(ap, "semid", &semid);
	ap_get_int(ap, "shmid", &shmid);
	ap_get_int(ap, "door_id", &door_id);
	
	printf("%d: I am door %d and have: semid %d, shmid %d, req_q %d, resp_q %d\n", 
			getpid(), door_id, semid, shmid, req_q, resp_q);
			
	// Attach shm
	int* capacity = shm_attach(shmid);
	
	// Door main loop
	while(1) {
		message_t msg = {};
		msq_rcv(req_q, &msg, sizeof(message_t), 0);
		printf("%d: I am door %d and have a message from person %ld\n", getpid(), door_id, msg.mtype);
		
		sleep(TIME_DOOR_RESP + (rand() % (TIME_DOOR_RESP / 2))); // Spend some time processing message
		
		if(msg.msg_type == REQUEST_EXIT) {
			sem_wait(semid, 0);
			printf("%d: I am door %d and I'm allowing person %ld to leave\n", getpid(), door_id, msg.mtype);
			*capacity = (*capacity + 1);
			printf("%d: Museum capacity remaining: %d\n", getpid(), (*capacity));
			msg.msg_type = ACCEPTED;
			sem_signal(semid, 0);
			msq_send(resp_q, &msg, sizeof(message_t));
		}
		else if(msg.msg_type == REQUEST_ENTER) {
			sem_wait(semid, 0);
			printf("%d: Museum capacity remaining: %d\n", getpid(), (*capacity));
			if(*capacity) { // If this person can enter
				*capacity = (*capacity - 1);
				msg.msg_type = ACCEPTED;
				printf("%d: I am door %d and I'm accepting person %ld\n", getpid(), door_id, msg.mtype);
			}
			else { // If this person can't enter
				msg.msg_type = REJECTED;
				printf("%d: I am door %d and I'm rejecting person %ld\n", getpid(), door_id, msg.mtype);
			}
			sem_signal(semid, 0);
			
			msq_send(resp_q, &msg, sizeof(message_t));
		}
		else {
			printf("%d: Error! Message on dooor %d was not request!\n", getpid(), door_id);
			exit(-1);
		}
		
	}
	 		
	shm_detach(capacity);
	ap_destroy(ap);
	exit(0);
}
*/
