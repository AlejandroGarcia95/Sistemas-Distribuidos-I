#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "msq.h"
#include <sys/types.h>
#include "sem.h"
#include "general.h"
#include "argv_parser.h"
#include "shm.h"



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
		
		usleep(TIME_DOOR_RESP + (rand() % (TIME_DOOR_RESP / 2))); // Spend some time processing message
		
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
