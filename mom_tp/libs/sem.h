#ifndef _SEM_H_
#define _SEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <errno.h>
#include <assert.h>
#include <sys/sem.h>

#define SEM_MAGIC_INT 6

//------------------------------ SEM USE ------------------------------- 
/*

			Parent process (initializer/destroyer)
            |
 sem_create |
            |
 sem_init   |
            |
            | [fork] 
            |	\
            |	 \
            |	  \  [exec] Give child process
            |	   \ the semid as argument
            |	    \  
 can use    |	     \
 sem_wait or|	      \
 sem_signal |	       \
 as desired |          Child process
            |          |
            |          | Can use sem_wait and
            |          | sem_signal as desired
            |          |
            |        [exit] 
            |-----------
sem_destroy |            
            |            

*/

int sem_create(size_t amount, char* file){
	key_t key = ftok(file, SEM_MAGIC_INT);
	if(key < 0) {
		printf("%d: Error creating semaphore: %d\n", getpid(), errno);
		exit(-1);
	} 
	int semid = semget(key, amount, IPC_CREAT|0644);
	if(semid < 0){
		printf("%d: Error creating semaphore: %d\n", getpid(), errno);
		exit(-1);
	}
	return semid;
}

void sem_destroy(int semid){
	if(semctl(semid, IPC_RMID,0) < 0){
		printf("%d: Error destroying semaphore: %d\n", getpid(), errno);
		exit(-1);
	}
}

void sem_init(int semid, size_t sem_num, int value){
	assert(value >= 0);
	if(semctl(semid, sem_num, SETVAL, value) < 0){
		printf("%d: Error initializing semaphore: %d\n", getpid(), errno);
		exit(-1);
	}
}

void sem_signal(int semid, size_t sem_num){
	struct sembuf sop = {sem_num, 1, 0};
	if(semop(semid, &sop, 1) < 0){
		printf("%d: Error signaling semaphore: %d\n", getpid(), errno);
		exit(-1);	
	}
}

void sem_wait(int semid, size_t sem_num){
	struct sembuf sop = {sem_num, -1, 0};
	if(semop(semid, &sop, 1) < 0){
		printf("%d: Error waiting semaphore: %d\n", getpid(), errno);
		exit(-1);	
	}
}

#endif /* _SEM_H_ */
