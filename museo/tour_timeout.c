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
		printf("%d: Error creating argv_parser:%d\n", getpid(), errno);
		exit(-1);
	}
	
	int shmid, semid, semid_vacancy;
	ap_get_int(ap, "shmid_tour", &shmid);
	ap_get_int(ap, "semid_tour", &semid);
	ap_get_int(ap, "semid_vacancy", &semid_vacancy);
				
	//printf("%d: I am the guide and I'm just spawned!\n", getpid());			
				
	// Attach shm
	int* people_tour = shm_attach(shmid);
	int i;
	// timeout main loop
	while(1) {
		usleep(TIME_TOUR_TIMEOUT); 
		// Timeout expired, so the tour begins
		
		// Lock shared memory so that other person aint signal semid
		sem_wait(semid_vacancy, 0);
		int c = (*people_tour) ;
		if((c < PEOPLE_TOUR) && (c > 0)) {
			printf("%d: Timeout for tour reached. Tour will start with fewer people.\n", getpid());
			sem_signal(semid, 0);
		}
		sem_signal(semid_vacancy, 0);
		
	}
	 		
	shm_detach(people_tour);
	ap_destroy(ap);
	exit(0);
}
