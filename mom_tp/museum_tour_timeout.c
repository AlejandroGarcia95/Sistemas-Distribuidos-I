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
		printf("%d: Error creating mom for tour timeout: %d\n", getpid(), errno);
		exit(-1);
	}
	
	sleep(2);
	subscribe_to_coordinator(mom);
				
	int i;
	// timeout main loop
	while(1) {
		printf("%d: Tour timeout is ticking\n", getpid());
		sleep(TIME_TOUR_TIMEOUT); 
		// Timeout expired, so the tour begins
		
		// Lock shared memory so that other person aint signal semid
		dsem_wait(mom, "sem_vacancy");
		int c;
		dshm_read(mom, "shm_tour", &c);
		
		if((c < PEOPLE_TOUR) && (c > 0)) {
			printf("%d: Timeout for tour reached. Tour will start with fewer people (%d)\n", getpid(), c);
			dsem_signal(mom, "sem_tour");
		}
		dsem_signal(mom, "sem_vacancy");
	}
	 		
	mom_destroy(mom);
	exit(0);
}

