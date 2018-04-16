#ifndef _MOM_H_
#define _MOM_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


typedef struct mom_ {
	int id;
	int msqid;
} mom_t;


mom_t* mom_create();

bool mom_publish(mom_t* mom, char* topic, const void *msg);

bool mom_subscribe(mom_t* mom, char* topic);

bool mom_receive(mom_t* mom, void* msg);

void mom_destroy(mom_t* mom);

#endif /* _MOM_H_ */
