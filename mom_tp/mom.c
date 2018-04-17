#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "mom.h"
#include "libs/msq.h"
#include "mom_general.h"


// ------------------- PRIVATE FUNCTIONS ---------------------------


void fill_message(mom_message_t* m, mom_t* mom, opcode_t opcode, char* topic, char* payload) {
	m->mtype = mom->local_id;
	m->opcode = opcode;
	m->sender_id = mom->local_id;
	if(topic)	strcpy((char*) m->topic, topic);
	if(payload)	strcpy((char*) m->payload, payload);
}

void send_message_to_daemon(mom_t* mom, mom_message_t m, mom_message_t* response) {
	// Send m
	msq_send(mom->msqid_requester, &m, sizeof(mom_message_t));
	// Receive answer and store into response
	msq_rcv(mom->msqid_responser, &response, sizeof(mom_message_t), mom->local_id);
}


// --------------- LIBRARY PUBLIC FUNCTIONS ------------------------

mom_t* mom_create(){
	mom_t* mom = malloc(sizeof(mom_t));
	if(!mom)	return NULL;
	
	mom->local_id = getpid(); // Genius
	
	// Retrieve dameon queues' ids
	mom->msqid_requester = msq_create(QUEUE_REQUESTER);
	mom->msqid_responser = msq_create(QUEUE_RESPONSER);
	// Write and read create message to retrieve mom->global_id
	mom_message_t m, response = {0};
	fill_message(&m, mom, OC_CREATE, NULL, NULL);
	send_message_to_daemon(mom, m, &response);
	// TODO: Retrieve mom->global_id from response
	mom->global_id = mom->local_id;
	return mom;
}

bool mom_publish(mom_t* mom, char* topic, const void *msg){
	mom_message_t m, response = {0};
	// Write and read publish message
	fill_message(&m, mom, OC_PUBLISH, topic, (char*) msg);
	send_message_to_daemon(mom, m, &response);
	if((response.opcode != OC_ACK_SUCCESS) && (response.opcode != OC_ACK_FAILURE)) {
		printf("%d: MOM CRITICAL: Daemon answer was not ACK!\n", getpid());
		return false;
	}
	
	return response.opcode == OC_ACK_SUCCESS;
}

bool mom_subscribe(mom_t* mom, char* topic){
	mom_message_t m, response = {0};
	// Write and read subscribe message
	fill_message(&m, mom, OC_SUBSCRIBE, topic, NULL);
	send_message_to_daemon(mom, m, &response);
	if((response.opcode != OC_ACK_SUCCESS) && (response.opcode != OC_ACK_FAILURE)) {
		printf("%d: MOM CRITICAL: Daemon answer was not ACK!\n", getpid());
		return false;
	}
	
	return response.opcode == OC_ACK_SUCCESS;
}

bool mom_receive(mom_t* mom, void* msg){
	// Locally receive message from daemon requester
	mom_message_t response = {0};
	msq_rcv(mom->msqid_responser, &response, sizeof(mom_message_t), mom->local_id);
	if(response.opcode != OC_DELIVERED) {
		printf("%d: MOM CRITICAL: Daemon has not delivered message!\n", getpid());
		return false;
	}
	// Copy received payload into msg
	strcpy((char*) msg, (char*) response.payload);
	return true;
}


void mom_destroy(mom_t* mom){
	if(!mom)	return;
	free(mom);
	// Must not destroy mom->msqids since they're mom_daemon queues
}
