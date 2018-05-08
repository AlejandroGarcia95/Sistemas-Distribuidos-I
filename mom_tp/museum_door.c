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
	ap_t* ap = ap_create_from_argv(argc, argv);
	if(!ap) {
		printf("%d: Error creating argv_parser: %d\n", getpid(), errno);
		exit(-1);
	}
	
	mom_t* mom = mom_create();
	if(!mom) {
		printf("%d: Error creating mom for door:%d\n", getpid(), errno);
		ap_destroy(ap);
		exit(-1);
	}	
	
	subscribe_to_coordinator(mom, PRIORITY_DOOR);
	
	int door_id;
	ap_get_int(ap, "door_id", &door_id);
	
	char my_topic[26];
	sprintf(my_topic, "Museum/Doors/Door%d", door_id);
	mom_subscribe(mom, my_topic);
	
	printf("%d: I am door %d and I'm ready!\n", getpid(), door_id);
	
	// Door main loop
	while(1) {
		char msg[15] = {0};
		int person_id, msg_code;
		int capacity;
		
		mom_receive(mom, msg);
		sscanf(msg, "%d %d", &person_id, &msg_code);
		
		printf("%d: I am door %d and have a message from person %d\n", getpid(), door_id, person_id);
		
		sleep(TIME_DOOR_RESP); // Spend some time processing message
		
		if(msg_code == CODE_REQUEST_EXIT) {
			dsem_wait(mom, "sem_capacity");
			printf("%d: I am door %d and I'm allowing person %d to leave\n", getpid(), door_id, person_id);
			dshm_read(mom, "shm_capacity", &capacity);
			capacity++;
			dshm_write(mom, "shm_capacity", capacity);
			printf("%d: Museum capacity remaining: %d\n", getpid(), capacity);
			
			sprintf(msg, "%d %d", door_id, CODE_ACCEPTED); // sender_id code_number
			char person_topic[26];
			sprintf(person_topic, "Museum/People/Person%d", person_id);
			
			dsem_signal(mom, "sem_capacity");
			mom_publish(mom, person_topic, msg);
		}
		else if(msg_code == CODE_REQUEST_ENTER) {
			dsem_wait(mom, "sem_capacity");
			dshm_read(mom, "shm_capacity", &capacity);
			printf("%d: Museum capacity remaining: %d\n", getpid(), capacity);
			if(capacity > 0) { // If this person can enter
				capacity--;
				dshm_write(mom, "shm_capacity", capacity);
				sprintf(msg, "%d %d", door_id, CODE_ACCEPTED); // sender_id code_number
				printf("%d: I am door %d and I'm accepting person %d\n", getpid(), door_id, person_id);
			}
			else { // If this person can't enter
				sprintf(msg, "%d %d", door_id, CODE_REJECTED); // sender_id code_number
				printf("%d: I am door %d and I'm rejecting person %d\n", getpid(), door_id, person_id);
			}
			dsem_signal(mom, "sem_capacity");
			
			char person_topic[26];
			sprintf(person_topic, "Museum/People/Person%d", person_id);
			mom_publish(mom, person_topic, msg);
		}
		else {
			printf("%d: Error! Message on dooor %d was not request!\n", getpid(), door_id);
			exit(-1);
		}
		
	}
	 		
	mom_destroy(mom);
	ap_destroy(ap);
	exit(0);
}
