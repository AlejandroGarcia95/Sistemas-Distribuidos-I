#ifndef _MUSEUMMM_H_
#define _MUSEUMMM_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "mom.h"

#define DOOR_AMOUNT 3
#define SIMULATE_FOREVER 0	// If setting this, increase delay timing
#define PEOPLE_AMOUNT 12
#define PEOPLE_TOUR 3
#define MUSEUM_CAPACITY 6
#define PERSON_PROB_SPAWNING 80
#define PERSON_PROB_TOUR 80
// Delay in seconds
#define TIME_PERSON_SPAWN 2
#define TIME_DOOR_RESP 1
#define TIME_PERSON_INSIDE 5
#define TIME_TOUR_TIMEOUT 10
#define TIME_TOUR_DURATION 7

#define CODE_ACCEPTED 1
#define CODE_REJECTED 2
#define CODE_REQUEST_ENTER 3
#define CODE_REQUEST_EXIT 4
#define CODE_REQUEST_TOUR 5


// Priorities for coordinator

#define PRIORITY_PERSON 1
#define PRIORITY_GUIDE 3
#define PRIORITY_DOOR 3
#define PRIORITY_TIMEOUT 2
#define PRIORITY_MAIN 5

// Comunication with coordinator of dshm and dsem

#define COORD_TOPIC "Museum/Coordinator"

#define COORD_MSG_SIZE 70

void subscribe_to_coordinator(mom_t* mom, int priority);

void dshm_init(mom_t* mom, char* name, int value);

void dshm_read(mom_t* mom, char* name, int* value);

void dshm_write(mom_t* mom, char* name, int value);

void dsem_init(mom_t* mom, char* name, int value);

void dsem_signal(mom_t* mom, char* name);

void dsem_wait(mom_t* mom, char* name);

void dsem_destroy(mom_t* mom, char* name);
void dshm_destroy(mom_t* mom, char* name);

#endif
