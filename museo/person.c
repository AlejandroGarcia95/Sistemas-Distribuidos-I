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

void take_tour(ap_t* ap, int person_id){
	int req_q, resp_q, shmid, semid, semid_vacancy;
	ap_get_int(ap, "tour_request", &req_q);
	ap_get_int(ap, "tour_response", &resp_q);
	ap_get_int(ap, "shmid_tour", &shmid);
	ap_get_int(ap, "semid_tour", &semid);
	ap_get_int(ap, "semid_vacancy", &semid_vacancy);
	
	int* people_tour = shm_attach(shmid);
	sem_wait(semid_vacancy, 0);
	*people_tour = *people_tour + 1;
	printf("%d: Person %d has signed for tour\n", getpid(), person_id);
	message_t msg = {person_id, REQUEST_TOUR};
	msq_send(req_q, &msg, sizeof(message_t));
	if((*people_tour) == PEOPLE_TOUR)
		sem_signal(semid, 0);
	sem_signal(semid_vacancy, 0);
	printf("%d: Person %d is waiting the guide\n", getpid(), person_id);
	
	msq_rcv(resp_q, &msg, sizeof(message_t), person_id);
	printf("%d: Person %d is going with the guide\n", getpid(), person_id);
}

void inside_museum(ap_t* ap, int person_id){
	if((rand() % 100) <= PERSON_PROB_TOUR)
		take_tour(ap, person_id);
	else
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
