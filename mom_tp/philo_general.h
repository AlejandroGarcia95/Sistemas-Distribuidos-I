#ifndef _PHILO_H_
#define _PHILO_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "mom.h"

#define PHILO_AMOUNT 40 // Odd value

#define ITEMS 8


// Comunication with coordinator of dshm and dsem

#define COORD_TOPIC "Philo/Coordinator"

#define PROXY_FILE_PY "new_proxy.py"

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
