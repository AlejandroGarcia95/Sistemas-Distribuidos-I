#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>
#include "lock.h"

#define CHILDREN_AMOUNT 18
#define CHILDREN_LOOP 5
#define LOCK_FILE "thelock.txt"

void children_main(){
	// Open lock fd
	int fd = lock_create(LOCK_FILE);
	int i;
	// Try to get lock, wait some time and release it
	for(i = 0; i < CHILDREN_LOOP; i++) {
		lock_acquire(fd, 1);
		printf("I am son %d and I have the lock\n", getpid());
		usleep(1000);
		printf("I am son %d and I will release the lock\n", getpid());
		lock_release(fd);
		usleep(100);
	}
	lock_destroy(fd);
	exit(0);
}


int main(int arg, char* argv[]){	
	// Create and acquire the lock
	int fd = lock_create(LOCK_FILE);
	lock_acquire(fd, 1);
	int i;
	// Launch children and let'em try get the lock
	for(i = 0; i < CHILDREN_AMOUNT; i++) {
		pid_t pid = fork();
		if(pid < 0) {
			printf("%d: Error launching child: %d\n", getpid(), errno);
			exit(-1);
		}
		if(pid == 0)
			children_main();
	}
	
	// Release the lock, allowing children to claim it
	printf("Alright children, you can have the lock\n");
	lock_release(fd);	
		
	// Wait for children
	for(i = 0; i < CHILDREN_AMOUNT; i++)
		wait(NULL);
	
	 // Destroy lock
	 lock_destroy(fd);
			
	return 0;
}
