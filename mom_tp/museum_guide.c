#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>

#include "mom.h"
#include "museum_general.h"
#include "libs/argv_parser.h"


int main(){
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
	
	int req_q, resp_q, shmid, semid, semid_vacancy;
	ap_get_int(ap, "tour_request", &req_q);
	ap_get_int(ap, "tour_response", &resp_q);
	ap_get_int(ap, "shmid_tour", &shmid);
	ap_get_int(ap, "semid_tour", &semid);
	ap_get_int(ap, "semid_vacancy", &semid_vacancy);
				
	printf("%d: I am the guide and I'm just spawned!\n", getpid());			
				
	// Attach shm
	int* people_tour = shm_attach(shmid);
	int i;
	// Guide main loop
	while(1) {
		sem_wait(semid, 0);
		// All people on tour
		sem_wait(semid_vacancy, 0);
		printf("%d: I am the guide and I'm taking %d people!\n", getpid(), *people_tour);
		for(i = 0; i < *people_tour; i++){
			message_t msg = {};
			msq_rcv(req_q, &msg, sizeof(message_t), 0);
			if(msg.msg_type != REQUEST_TOUR) {
				printf("%d: Error! Message on tour was not request!\n", getpid());
			}
			msg.msg_type = ACCEPTED;
			msq_send(resp_q, &msg, sizeof(message_t));
		}
		*people_tour = 0;
		sem_signal(semid_vacancy, 0);
		
	}
	 		
	shm_detach(people_tour);
	ap_destroy(ap);
	exit(0);
}
*/
