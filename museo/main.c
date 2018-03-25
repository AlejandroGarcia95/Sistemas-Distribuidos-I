#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "sem.h"
#include "msq.h"
#include "shm.h"
#include "general.h"
#include "argv_parser.h"

/* Creates a temporal file with a given temp_string and temp_id.
 * The resulting file name is formatted as
 * "%s-%d.temp", temp_string, temp_id
 * If buff is not NULL, then the resulting file name is copied
 * into it. Max temporal file name length is 50.*/
void create_temporal(char* temp_string, int temp_id, char* buff){
	char temp_buff[50];
	sprintf(temp_buff, "%s-%d.temp", temp_string, temp_id);
	if(mknod(temp_buff, S_IFREG|0666, 0) < 0) {
			printf("%d: Error creating temporal file %s: %d\n", getpid(), temp_buff, errno);
			exit(-1);
		}
	
	if(buff)
		sprintf(buff, "%s", temp_buff);
}

void launch_door_process(ap_t* ap_people, int door_id, int semid, int shmid, pid_t* doors_pids){
	// request_queue: where people ask door to enter/exit
	// response_queue: where door answers people
	
	// First create temporary files
	char buff_req[30], buff_resp[30];
	create_temporal("req_q", door_id, buff_req);
	create_temporal("resp_q", door_id, buff_resp);
	
	// Create queues and get ids
	int request_queue = msq_create(buff_req);
	int response_queue = msq_create(buff_resp);

	// Prepare to launch door child
	
	ap_t* ap_door = ap_create("./door");
	if(!ap_door) {
		printf("%d: Error creating argv_parser for door:%d\n", getpid(), errno);
		exit(-1);
	}
	
	ap_set_int(ap_door, "request_queue", request_queue);
	ap_set_int(ap_door, "response_queue", response_queue);
	ap_set_int(ap_door, "shmid", shmid);
	ap_set_int(ap_door, "semid", semid);
	ap_set_int(ap_door, "door_id", door_id);

	// Launch door!
	pid_t pid = fork();
	if (pid < 0) {
		printf("%d: Error forking: %d\n", getpid(), errno);
		exit(-1);
	}
	if (pid == 0) {
		ap_exec(ap_door);
	}
	
	doors_pids[door_id] = pid;
	ap_destroy(ap_door);
	
	// Add the door request and response queues to ap_people
	char req_q_id[40], resp_q_id[40];
	sprintf(req_q_id, "request_queue_%d", door_id);
	sprintf(resp_q_id, "response_queue_%d", door_id);
	ap_set_int(ap_people, req_q_id, request_queue);
	ap_set_int(ap_people, resp_q_id, response_queue);
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
	}
}

int main(int argc, char* argv[]) {
	int semid = sem_create(1, "main.c");
	sem_init(semid, 0, 1);
	
	int shmid = shm_create("main.c", sizeof(int));
	
	int* capacity = (int*) shm_attach(shmid);
	*capacity = MUSEUM_CAPACITY;
	
	ap_t* ap_people = ap_create("./person");
		if(!ap_people) {
			printf("%d: Error creating argv_parser for people:%d\n", getpid(), errno);
			exit(-1);
		}
	
	int i;
	// Launch all doors
	pid_t doors_pids[DOOR_AMOUNT] = {};
    for (i = 0; i < DOOR_AMOUNT; i++)
		launch_door_process(ap_people, i, semid, shmid, doors_pids);

	// Emulate people spawning
	int people_spawned = 0;
	bool keep_simulating = 1;
	while(keep_simulating) {
		// Every 1000 microseconds, launch person with probability PERSON_PROB_SPAWNING
		usleep(1000);
		if((rand() % 100) <= PERSON_PROB_SPAWNING) {
			people_spawned++;
			launch_person_process(ap_people, people_spawned);
		}
		keep_simulating = SIMULATE_FOREVER || (people_spawned < PEOPLE_AMOUNT);
	}
	

	// If here, wait people
    for (i = 0; i < PEOPLE_AMOUNT; i++)
		wait(NULL);
	
	// Kill all doors
	for (i = 0; i < DOOR_AMOUNT; i++) 
		kill(doors_pids[i], SIGTERM);
	
	shm_detach(capacity);
	// If here, destroy all IPC
	shm_destroy(shmid);
	sem_destroy(semid);
	for(i = 0; i < DOOR_AMOUNT; i++){
		// Destroy all queues
		char req_q_name[40], resp_q_name[40];
		sprintf(req_q_name, "request_queue_%d", i);
		sprintf(resp_q_name, "response_queue_%d", i);
		int req_q, resp_q;
		ap_get_int(ap_people, req_q_name, &req_q);
		ap_get_int(ap_people, resp_q_name, &resp_q);
		msq_destroy(req_q);
		msq_destroy(resp_q);
	}
	
	ap_destroy(ap_people);
	return 0;
}
	
