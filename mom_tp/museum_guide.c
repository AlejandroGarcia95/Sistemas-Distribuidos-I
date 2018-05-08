#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <errno.h>

#include "mom.h"
#include "museum_general.h"
#include "libs/argv_parser.h"


int main(int argc, char* argv[]) {
	srand(time(NULL)); 

	mom_t* mom = mom_create();
	if(!mom) {
		printf("%d: Error creating mom for guide: %d\n", getpid(), errno);
		exit(-1);
	}
	
	mom_subscribe(mom, "Museum/Guide");
	subscribe_to_coordinator(mom, PRIORITY_GUIDE);
					
	printf("%d: I am the guide and I've just spawned!\n", getpid());						

	int i;
	// Guide main loop
	while(1) {
		dsem_wait(mom, "sem_tour");
		// All people on tour
		dsem_wait(mom, "sem_vacancy");
		int ppl_tour = 0;
		dshm_read(mom, "shm_tour", &ppl_tour);
		if(ppl_tour > 0) // Should always happen
			printf("%d: I am the guide and I'm taking %d people!\n", getpid(), ppl_tour);
		for(i = 0; i < ppl_tour; i++){
			// Read person request
			char msg[15] = {0};
			mom_receive(mom, msg);
			int person_id, msg_code;
			sscanf(msg, "%d %d", &person_id, &msg_code);
			
			if(msg_code != CODE_REQUEST_TOUR) {
				printf("%d: Error! Message on tour was not request (was %s)!\n", getpid(), msg);
			}

			// Send reply
			sprintf(msg, "%d %d", DOOR_AMOUNT + 1, CODE_ACCEPTED); // sender_id code_number
			char person_topic[26];
			sprintf(person_topic, "Museum/People/Person%d", person_id);
			mom_publish(mom, person_topic, msg);
			//dsem_signal(mom, "sem_guide");
		}
		dshm_write(mom, "shm_tour", 0);
		dsem_signal(mom, "sem_vacancy");
				
	}
	 		
	mom_destroy(mom);
	exit(0);
}
