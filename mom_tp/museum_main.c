#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "mom.h"
#include "museum_general.h"
#include "libs/argv_parser.h"


void launch_door_process(int door_id, pid_t* children_pids){
	// Prepare to launch door child
	ap_t* ap_door = ap_create("./museum_door");
	if(!ap_door) {
		printf("%d: Error creating argv_parser for door:%d\n", getpid(), errno);
		exit(-1);
	}
	
	ap_set_int(ap_door, "door_id", door_id);

	// Launch door!
	pid_t pid = fork();
	if (pid < 0) {
		printf("%d: Error forking: %d\n", getpid(), errno);
		exit(-1);
	}
	if (pid == 0) {
		ap_exec(ap_door);
		printf("DAMN ITS HERE---DOOR\n");
		exit(-1);
	}
	
	children_pids[door_id] = pid;
	ap_destroy(ap_door);
}

void launch_person_process(ap_t* ap_people, int person_id){
	srand(time(NULL)); 
	ap_t* ap_this_person = ap_clone(ap_people);
	ap_set_int(ap_this_person, "Person id", person_id);
	// Launch person!
	int pid = fork();
	if (pid < 0) {
		printf("%d: Error forking: %d\n", getpid(), errno);
		exit(-1);
	}
	if (pid == 0) {
		ap_exec(ap_this_person);
		printf("DAMN ITS HERE---PERSON\n");
		exit(-1);
	}
	
	ap_destroy(ap_this_person);
}


void launch_tour_timeout(pid_t* children_pids){
	// Launch tour_timeout!
	ap_t* ap_timeout = ap_create("./museum_tour_timeout");
	 
	ap_set_string(ap_timeout, "me", "./museum_tour_timeout");                             
	if(!ap_timeout) {
		printf("%d: Error creating argv_parser for tour timeout:%d\n", getpid(), errno);
		exit(-1);
	}
	
	pid_t pid = fork();
	if (pid < 0) {
		printf("%d: Error forking: %d\n", getpid(), errno);
		exit(-1);
	}
	if (pid == 0) {
		char *const parm_list[] = {"./museum_tour_timeout", NULL};
		execv("./museum_tour_timeout", parm_list);
		printf("DAMN ITS HERE---TIMEOUT: %d\n", errno);
		exit(-1);
	}
	
	children_pids[DOOR_AMOUNT + 1] = pid;
	ap_destroy(ap_timeout);
}

void launch_guide(pid_t* children_pids){
	// Prepare to launch guide child
	ap_t* ap_guide = ap_create("./museum_guide");
	if(!ap_guide) {
		printf("%d: Error creating argv_parser for guide:%d\n", getpid(), errno);
		exit(-1);
	}

	// Launch guide!
	pid_t pid = fork();
	if (pid < 0) {
		printf("%d: Error forking: %d\n", getpid(), errno);
		exit(-1);
	}
	if (pid == 0) {
		ap_exec(ap_guide);
		printf("DAMN ITS HERE---GUIDE\n");
		exit(-1);
	}
	
	launch_tour_timeout(children_pids);
	
	children_pids[DOOR_AMOUNT] = pid;
	ap_destroy(ap_guide);
}


int main(int argc, char* argv[]) {
	mom_t* mom = mom_create();
	if(!mom) {
		printf("%d: Error creating mom on museum_main: %d\n", getpid(), errno);
		return -1;
	}
	
	subscribe_to_coordinator(mom);
	
	dsem_init(mom, "sem_capacity", 1);
	dshm_init(mom, "shm_capacity", MUSEUM_CAPACITY);
	dsem_init(mom, "sem_tour", 0);
	dshm_init(mom, "shm_tour", 0);
	dsem_init(mom, "sem_vacancy", 1);
	
	ap_t* ap_people = ap_create("./museum_person");
		if(!ap_people) {
			printf("%d: Error creating argv_parser for people:%d\n", getpid(), errno);
			exit(-1);
		}
	
	int i;
	// Launch all doors
	pid_t children_pids[DOOR_AMOUNT + 2] = {}; // doors + guide + tour_timeout
    for (i = 0; i < DOOR_AMOUNT; i++)
		launch_door_process(i, children_pids);


	launch_guide(children_pids);
	// Emulate people spawning
	sleep(1);	// TODO: Remove this
	int people_spawned = 0;
	bool keep_simulating = 1;
	while(keep_simulating) {
		// Every TIME_PERSON_SPAWN microseconds, launch person 
		// with probability PERSON_PROB_SPAWNING
		sleep(TIME_PERSON_SPAWN);
		if((rand() % 100) <= PERSON_PROB_SPAWNING) {
			people_spawned++;
			launch_person_process(ap_people, people_spawned);
		}
		keep_simulating = SIMULATE_FOREVER || (people_spawned < PEOPLE_AMOUNT);
	}
	

	// If here, wait people
    for(i = 0; i < PEOPLE_AMOUNT; i++)
		wait(NULL);
	
	// Kill all doors
	for(i = 0; i < DOOR_AMOUNT + 2; i++) 
		kill(children_pids[i], SIGINT);
	
	dsem_destroy(mom, "sem_capacity");
	dshm_destroy(mom, "shm_capacity");
	dsem_destroy(mom, "sem_tour");
	dshm_destroy(mom, "shm_tour");
	dsem_destroy(mom, "sem_vacancy");
	
	ap_destroy(ap_people);
	printf("%d: The museum has closed its doors!\n", getpid());
	
	return 0;
}
