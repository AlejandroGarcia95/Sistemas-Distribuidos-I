#ifndef _SHM_H_
#define _SHM_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#define SHM_MAGIC_INT 8


//------------------------------ SHM USE ------------------------------- 
/*

			Parent process (initializer/destroyer)
            |
 shm_create |
            |
            | [fork] 
            |	\
            |	 \
            |	  \  [exec] Give child process
            |	   \ the shmid as argument
            |	    \  
 Can use    |	     \
 shm_attach |	      \
 if desired |	       \
 but must   |          Child process
 shm_detach |          |
 if used    |          | shm_attach
            |          |
            |          | Can use shm as desired
            |          |
            |          | shm_detach
            |          |
            |        [exit] 
            |-----------
shm_destroy |            
            |            

*/


int shm_create(char* file, size_t size){
	key_t key = ftok(file, SHM_MAGIC_INT);
	if(key < 0) {
		printf("%d: Error creating shared memory: %d\n", getpid(), errno);
		exit(-1);
	} 
	int shmid = shmget(key, size, IPC_CREAT|0644);
	if(shmid < 0){
		printf("%d: Error creating shared memory: %d\n", getpid(), errno);
		exit(-1);
	}
	return shmid;
}

void* shm_attach(int shmid){
	void* p = shmat(shmid, NULL, 0);
	if(p == (void*)-1){
		printf("%d: Error attaching shared memory: %d\n", getpid(), errno);
		exit(-1);
	}
	return p;
}

void shm_detach(void* p){
	if(shmdt(p) < 0){
		printf("%d: Error detaching shared memory: %d\n", getpid(), errno);
		exit(-1);
	}
}

void shm_destroy(int shmid){
	if(shmctl(shmid, IPC_RMID,0) < 0){
		printf("%d: Error destroying shared memory: %d\n", getpid(), errno);
		exit(-1);
	}
}

#endif /* _SHM_H_ */
