#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "mom.h"
#include "libs/msq.h"

mom_t* mom_create(){
	mom_t* mom = malloc(sizeof(mom_t));
	if(!mom)	return NULL;
	
	mom->msqid = msq_create("mom.h");
	// TODO: Write and read register message through
	// mom->msqid to retrieve mom->id
	mom->id = 6;
	return mom;
}

bool mom_publish(mom_t* mom, char* topic, const void *msg){
	// TODO: Write and read publish message through mom->msqid
	return false;
}

bool mom_subscribe(mom_t* mom, char* topic){
	// TODO: Write and read subscribe message through mom->msqid
	return false;
}

bool mom_receive(mom_t* mom, void* msg){
	// TODO: Write and read receive message through mom->msqid
	return false;
}


void mom_destroy(mom_t* mom){
	if(!mom)	return;
	free(mom);
	// Must not destroy mom->msqid since it's mom_daemon queue
}
