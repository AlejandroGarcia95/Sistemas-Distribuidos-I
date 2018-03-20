#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "shm.h"


#define SHM_SIZE 10
#define CHILDREN_AMOUNT 36
#define CHILDREN_LOOP 10

void children_main(int shmid){
	// Attach shm
	int* p = (int*) shm_attach(shmid);
	int i;
	
	// Increment shm location based on pid
	for(i = 0; i < CHILDREN_LOOP; i++) {
		usleep(1000);
		p[getpid() % SHM_SIZE]++;
	}
	
	// Detach shm
	shm_detach(p);
	printf("Son %d finished\n", getpid());
	exit(0);
}


int main(int arg, char* argv[]){
	// Create shared memory
	int shmid = shm_create("shm_test.c", SHM_SIZE * sizeof(int));
	int i;
	// Attach shm to initialize values
	int* p = (int*) shm_attach(shmid);
	for(i = 0; i < SHM_SIZE; i++)
		p[i] = 0;
	
	// Launch children to increment shared memory
	for(i = 0; i < CHILDREN_AMOUNT; i++) {
		pid_t pid = fork();
		if(pid < 0) {
			printf("%d: Error launching child: %d\n", getpid(), errno);
			exit(-1);
		}
		if(pid == 0)
			children_main(shmid);
	}
		
	// Wait for children
	for(i = 0; i < CHILDREN_AMOUNT; i++)
		wait(NULL);
		
	// Print shm values
	for(i = 0; i < SHM_SIZE; i++)
		printf("%d: Memory element %d is %d\n", getpid(), i, p[i]);
		
	// Detach shm
	shm_detach(p);
	
	 // Destroy shm
	 shm_destroy(shmid);
			
	return 0;
}
