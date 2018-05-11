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
#include "philo_general.h"
#include "libs/argv_parser.h"


/* Producer-consumer philosophers problem: We have PHILO_AMOUNT philosophers 
 sitting on a round table, trying to acquire two chopsticks, one at their 
 left and one at their right. Once a philosopher have two chopsticks, they 
 can either produce or consume an item. This behaviour is defined by the
 philosopher's id: odd philosophers will produce for the philosopher after
 them, and even philosophers will consume from the philospher before them
 (i.e. 0 will produce for 1 and 1 will consume from 0 and so on). Philosophers
 ids go from 0 to PHILO_AMOUNT-1 in clockwise sense. Once a philosopher do
 their consumption or production, they release the chopsticks. The cycle is
 repeated ITEMS times.
 
 
 Implementation: Each philospher will be a process with id ranging from
 0 to PHILO_AMOUNT-1. Each chopstick will be a 1-initialized semaphore,
 with name "SEMSTICK_x" with x ranging from 0 to PHILO_AMOUNT-1. As such,
 each philosopher with id "i" must wait for both sems "SEMSTICK_i" and
 "SEMSTICK_((i-1)%PHILO_AMOUNT)". Each buffer for storing items will be
 a shared memory with name "SHMBUFFER_x" with x being the id of the PRODUCER
 philosopher (i.e. philosopher 0 will produce to "SHMBUFFER_0" and philosopher
 1 will consume from the same buffer). Note we need no extra semaphore for
 locking the buffer (chopsticks' semaphores already accomplish that).
*/

void launch_philo(ap_t* ap, int id) {
	ap_t* ap_philo = ap_clone(ap);
	if(!ap_philo) {
		printf("%d: Error creating argv_parser for philo %d: %d\n", getpid(), id, errno);
		exit(-1);
	}
	
	ap_set_int(ap_philo, "id", id);
	
	pid_t pid = fork();
    if (pid < 0) {
        printf("%d: Error forkig philo %d: %d\n", getpid(), id, errno);
        return;
    }

    if (pid == 0) {
        // Philosopher
        ap_exec(ap_philo);
        printf("SHOULDNT BE HERE %d\n", id);
        exit(-1);
    }
    
    ap_destroy(ap_philo);
}


int main(int argc, char* argv[]) {
	mom_t* mom = mom_create();
	if(!mom) {
		printf("%d: Error creating mom on main: %d\n", getpid(), errno);
		return -1;
	}
	
	ap_t* ap = ap_create("./philosopher");
	if(!ap) {
		printf("%d: Error creating argv_parser: %d\n", getpid(), errno);
		mom_destroy(mom);
		return -1;
	}
	
	subscribe_to_coordinator(mom, 1);
	
	// Create chopsticks' semaphores
	char sem_name[20] = {0};
	for(int i = 0; i < PHILO_AMOUNT; i++) {
		sprintf(sem_name, "SEMSTICK_%d", i);
		dsem_init(mom, sem_name, 1);
	}
	
	// Create buffers
	char shm_name[20] = {0};
	for(int i = 0; i < PHILO_AMOUNT; i+=2) {
		sprintf(shm_name, "SHMBUFFER_%d", i);
		dshm_init(mom, shm_name, 0);
	}
	
	printf("%d: ------------- Starting simulation -------------\n", getpid());
	
	// Launch all philosophers
	for(int i = 0; i < PHILO_AMOUNT; i++)
		launch_philo(ap, i);
	
	// Wait all philosophers
	for(int i = 0; i < PHILO_AMOUNT; i++)
		wait(NULL);
	
	
	// Destroy shared resources
		for(int i = 0; i < PHILO_AMOUNT; i++) {
		sprintf(sem_name, "SEMSTICK_%d", i);
		dsem_destroy(mom, sem_name);
	}
	
	for(int i = 0; i < PHILO_AMOUNT; i+=2) {
		sprintf(shm_name, "SHMBUFFER_%d", i);
		dshm_destroy(mom, shm_name);
	}
	
	ap_destroy(ap);
	mom_destroy(mom);
	
	printf("%d: ------------- Simulation finished -------------\n", getpid());
	
	return 0;
}
