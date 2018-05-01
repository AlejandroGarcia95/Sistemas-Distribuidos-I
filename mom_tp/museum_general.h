#ifndef _MUSEUMMM_H_
#define _MUSEUMMM_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "mom.h"

#define DOOR_AMOUNT 3
#define SIMULATE_FOREVER 0	// If setting this, increase delay timing
#define PEOPLE_AMOUNT 26
#define PEOPLE_TOUR 6
#define MUSEUM_CAPACITY 12
#define PERSON_PROB_SPAWNING 80
#define PERSON_PROB_TOUR 80
// Delay in microseconds
#define TIME_PERSON_SPAWN 1000
#define TIME_DOOR_RESP 1500
#define TIME_PERSON_INSIDE 10000
#define TIME_TOUR_TIMEOUT 60000
#define TIME_TOUR_DURATION 8000

#define ACCEPTED 1
#define REJECTED 2
#define REQUEST_ENTER 3
#define REQUEST_EXIT 4
#define REQUEST_TOUR 5

// Comunication with coordinator of dshm and dsem

#define COORD_TOPIC "Museum/Coordinator"

#define COORD_MSG_SIZE 70


void dshm_init(mom_t* mom, char* name, int value);

void dshm_read(mom_t* mom, char* name, int* value);

void dshm_write(mom_t* mom, char* name, int value);

void dsem_init(mom_t* mom, char* name, int value);

void dsem_signal(mom_t* mom, char* name);

void dsem_wait(mom_t* mom, char* name);

#endif
