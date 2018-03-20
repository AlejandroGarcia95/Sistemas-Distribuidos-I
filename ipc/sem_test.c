#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "sem.h"

#define SEM_START_VALUE 3
#define CHILDREN_AMOUNT 5
#define CHILDREN_LOOP 10

void children_main(int semid){
	// First sleep a little
	usleep(1000);
	int i;
	
	// Loop while trying to wait and signal sem
	for(i = 0; i < CHILDREN_LOOP; i++) {
		sem_wait(semid, 0);
		printf("I am child %d and I have waited the sem!\n", getpid());
		usleep(1000);
		printf("I am child %d and I'll signal the sem\n", getpid());
		sem_signal(semid, 0);
		usleep(100);
	}
	
	exit(0);
}


int main(int arg, char* argv[]){
	// Create and initialize sem
	int semid = sem_create(1, "sem_test.c");
	sem_init(semid, 0, SEM_START_VALUE);
	int i;
	
	// Launch children to wait sem
	for(i = 0; i < CHILDREN_AMOUNT; i++) {
		pid_t pid = fork();
		if(pid < 0) {
			printf("%d: Error launching child: %d\n", getpid(), errno);
			exit(-1);
		}
		if(pid == 0)
			children_main(semid);
	}
		
	// Wait for children
	for(i = 0; i < CHILDREN_AMOUNT; i++)
		wait(NULL);
		
	 // Destroy sem
	 sem_destroy(semid);
			
	return 0;
}
