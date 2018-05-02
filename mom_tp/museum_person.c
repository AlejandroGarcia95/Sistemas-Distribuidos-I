#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include "mom.h"
#include "museum_general.h"
#include "libs/argv_parser.h"


void take_tour(mom_t* mom, int person_id){
	dsem_wait(mom, "sem_vacancy");
	int ppl_tour;
	dshm_read(mom, "shm_tour", &ppl_tour);
	ppl_tour++;
	dshm_write(mom, "shm_tour", ppl_tour);
	printf("%d: Person %d has signed for tour (%d are waiting)\n", getpid(), person_id, ppl_tour);
	
	//dsem_wait(mom, "sem_guide");
	
	char msg[15] = {0};
	sprintf(msg, "%d %d", person_id, CODE_REQUEST_TOUR); // sender_id code_number
	mom_publish(mom, "Museum/Guide", msg);
	
	if(ppl_tour == PEOPLE_TOUR) {
		printf("%d: Tour party is full (%d people). Telling the guide!\n", getpid(), ppl_tour);
		dsem_signal(mom, "sem_tour");
	}
	dsem_signal(mom, "sem_vacancy");
	printf("%d: Person %d is waiting the guide\n", getpid(), person_id);
	
	mom_receive(mom, msg);
	printf("%d: Person %d is going with the guide\n", getpid(), person_id);
	sleep(TIME_TOUR_DURATION);
}

void inside_museum(mom_t* mom, int person_id){
	if((rand() % 100) <= PERSON_PROB_TOUR)
		take_tour(mom, person_id);
	else
		sleep(TIME_PERSON_INSIDE + (rand() % TIME_PERSON_INSIDE)); // Spend some time inside museum
	// Randomly choose door
	int door_id = rand() % DOOR_AMOUNT;
	char door_topic[26];
	sprintf(door_topic, "Museum/Doors/Door%d", door_id);

	// Send request to exit
	char msg[15] = {0};
	sprintf(msg, "%d %d", person_id, CODE_REQUEST_EXIT); // sender_id code_number
	
	printf("%d: Person %d requesting to exit through door %d\n", getpid(), person_id, door_id);
	mom_publish(mom, door_topic, msg);
	
	// Receive response
	mom_receive(mom, msg);
}

int main(int argc, char* argv[]) {
	ap_t* ap = ap_create_from_argv(argc, argv);
	srand(getpid()); 

	if(!ap) {
		printf("Error creating argv_parser:%d\n", errno);
		exit(-1);
	}
		
	mom_t* mom = mom_create();
	if(!ap) {
		printf("%d: Error creating mom for person: %d\n", getpid(), errno);
		ap_destroy(ap);
		exit(-1);
	}	
	
	subscribe_to_coordinator(mom, PRIORITY_PERSON);
	
	int person_id;
	ap_get_int(ap, "Person id", &person_id);
		
	char my_topic[26];
	sprintf(my_topic, "Museum/People/Person%d", person_id);
	mom_subscribe(mom, my_topic);
	
	printf("%d: Person %d has just spawned\n", getpid(), person_id);
	
	// Randomly choose door
	int door_id = rand() % DOOR_AMOUNT;
	char door_topic[26];
	sprintf(door_topic, "Museum/Doors/Door%d", door_id);
	
	// Send request to enter
	char msg[15] = {0};
	sprintf(msg, "%d %d", person_id, CODE_REQUEST_ENTER); // sender_id code_number
	
	printf("%d: Person %d requesting to enter through door %d\n", getpid(), person_id, door_id);
	mom_publish(mom, door_topic, msg);
	
	// Receive response
	mom_receive(mom, msg);
	int msg_code;
	sscanf(msg, "%d %d", &door_id, &msg_code);
	if(msg_code == CODE_ACCEPTED) {
		printf("%d: Person %d was accepted by door %d\n", getpid(), person_id, door_id);
		inside_museum(mom, person_id);		
	}
	else {
		printf("%d: Person %d was rejected by door %d\n", getpid(), person_id, door_id);
	}

	mom_destroy(mom);
	ap_destroy(ap);
	exit(0);
}

