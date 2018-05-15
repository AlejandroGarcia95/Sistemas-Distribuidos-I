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

void philo_produce(mom_t* mom, int id) {
	char buff[20] = {0};
	sprintf(buff, "SHMBUFFER_%d", id);
	
	int value;
	dshm_read(mom, buff, &value);
	value++;
	dshm_write(mom, buff, value);
	printf("%d: Philo%d: I've produced a new item. Now there are %d\n", getpid(), id, value);
}


bool philo_consume(mom_t* mom, int id) {
	char buff[20] = {0};
	sprintf(buff, "SHMBUFFER_%d", id - 1);
	
	int value;
	dshm_read(mom, buff, &value);
	if(value > 0) {
		value--;
		dshm_write(mom, buff, value);
		printf("%d: Philo%d: I've consumed an item. Now there are %d\n", getpid(), id, value);
		return true;
	}
	else {
		printf("%d: Philo%d: I can't consume an item, there's none!\n", getpid(), id);
		return false;
	}
}


void philo_main(mom_t* mom, int id) {
	char first_stick[20] = {0};
	char second_stick[20] = {0};
	
	
	
	if(id == 0) { // If I'm the first, change order to avoid deadlock
		sprintf(first_stick, "SEMSTICK_%d", PHILO_AMOUNT - 1); 
		sprintf(second_stick, "SEMSTICK_0");
	}
	else {
		sprintf(first_stick, "SEMSTICK_%d", id);
		sprintf(second_stick, "SEMSTICK_%d", id - 1);
	}
	
	for(int i = 0; i < ITEMS; i++) {
		dsem_wait(mom, first_stick);
		printf("%d: Philo%d: I have my first chopstick\n", getpid(), id);
		dsem_wait(mom, second_stick);
		printf("%d: Philo%d: I have my second chopstick\n", getpid(), id);
		
		sleep(1 + (rand() % 3));
		if(id % 2) {
			if(!philo_consume(mom, id))
				i--;
		}
		else
			philo_produce(mom, id);
		
		printf("%d: Philo%d: I'm leaving my first chopstick\n", getpid(), id);
		dsem_signal(mom, first_stick);
		printf("%d: Philo%d: I'm leaving my second chopstick\n", getpid(), id);
		dsem_signal(mom, second_stick);
		sleep(1 + (rand() % 3));
	}
	
}


int main(int argc, char* argv[]) {
	srand(getpid()); 
	mom_t* mom = mom_create();
	if(!mom) {
		printf("%d: Error creating mom on philo: %d\n", getpid(), errno);
		return -1;
	}
	
	ap_t* ap = ap_create_from_argv(argc, argv);
	if(!ap) {
		printf("%d: Error creating argv_parser: %d\n", getpid(), errno);
		mom_destroy(mom);
		return -1;
	}
	
	int id;
	ap_get_int(ap, "id", &id);
	
	subscribe_to_coordinator(mom, id);
	
	
	printf("%d: Philo%d: I have spawned!\n", getpid(), id);
	
	
	philo_main(mom, id);
	
	ap_destroy(ap);
	mom_destroy(mom);
	
	printf("%d: Philo%d: I have died!\n", getpid(), id);
	
	return 0;
}
