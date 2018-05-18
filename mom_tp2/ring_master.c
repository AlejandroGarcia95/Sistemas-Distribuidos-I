#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include "mom_general.h"
#include "libs/msq.h"
#include "libs/socket.h"
#include "libs/argv_parser.h"

/* One ring to rule them all.
 One ring to find them.
 One ring to bring them all
 and in the darkness bind them.*/

typedef struct rm_data {
	char next_ip[20];
	char next_port[10];
	size_t broker_id;
	size_t broker_amount;
	int msqid_dbms;
	int msqid_rm;
	socket_t* s3;
} rm_data_t;


bool keep_looping = true;


void handler(int signum) {
  keep_looping = false;
}

void set_handler() {
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = handler;
	sigaction(SIGINT, &sa, NULL);
}

socket_t* establish_connection(rm_data_t rmd){
	// Create socket
	socket_t *s3 = socket_create(SOCK_ACTIVE);
	if(!s3){
		printf("%d: Error creating socket", getpid());
		return NULL; 
	}
	
	mom_message_t m = {0};
	
	if(rmd.broker_id == rmd.broker_amount) { // If I'm last
		socket_conect(s3, rmd.next_ip, rmd.next_port);
		// Send BR_CONNECT message
		m.opcode = OC_BR_CONNECT;
		m.mtype = rmd.broker_id;
		SOCKET_S(s3, mom_message_t, m);
		printf("%d: Connected with next broker instance!\n", getpid());
		// Wait for a BR_CONNECT message
		m = (mom_message_t) {0};
		msq_rcv(rmd.msqid_rm, &m, sizeof(mom_message_t), 0);
		if(m.opcode != OC_BR_CONNECT) {
			printf("%d: Critical error on ring_master: opcode was not BR_CONNECT\n", getpid());
			print_message(m);
			socket_destroy(s3);
			return NULL;
		}
		printf("%d: Previous broker made contact! Ring is done!\n", getpid());
		return s3;
	}
	else {
		// Wait for a BR_CONNECT message
		msq_rcv(rmd.msqid_rm, &m, sizeof(mom_message_t), 0);
		if(m.opcode != OC_BR_CONNECT) {
			printf("%d: Critical error on ring_master: opcode was not BR_CONNECT\n", getpid());
			print_message(m);
			socket_destroy(s3);
			return NULL;
		}
		printf("%d: Previous broker made contact. Contacting next broker...\n", getpid());
		socket_conect(s3, rmd.next_ip, rmd.next_port);
		// Send BR_CONNECT message
		m = (mom_message_t) {0};
		m.opcode = OC_BR_CONNECT;
		m.mtype = rmd.broker_id;
		SOCKET_S(s3, mom_message_t, m);
		printf("%d: Connected with next broker instance!\n", getpid());
		return s3;
	}
	
	return s3;
}

int main(int argc, char* argv[]) {
	ap_t* ap = ap_create_from_argv(argc, argv);
	if(!ap) {
		printf("%d: Error creating argv_parser:%d\n", getpid(), errno);
		exit(-1);
	}
	
	rm_data_t rmd = {0};

	ap_get_int(ap, QUEUE_HANDLER, &rmd.msqid_dbms);
	ap_get_int(ap, QUEUE_RM, &rmd.msqid_rm);
	ap_get_int(ap, BROKER_ID, &rmd.broker_id);
	ap_get_int(ap, BROKER_AMOUNT, &rmd.broker_amount);
	ap_get_string(ap, NEXT_IP, rmd.next_ip);
	ap_get_string(ap, NEXT_PORT, rmd.next_port);
	
	ap_destroy(ap);
	
	set_handler();
	printf("Ring master is up (PID: %d) !\n", getpid());
	
	rmd.s3 = establish_connection(rmd);
	
	if(!rmd.s3)	exit(-1);
	
	while(keep_looping) {
		mom_message_t m = {0};
		msq_rcv(rmd.msqid_rm, &m, sizeof(mom_message_t), 0);
		printf("%d: Ring master received a message!\n", getpid());
		if(m.opcode == OC_PUBLISH){
			// Forward to next broker instance
			m.global_id = -rmd.broker_id;
			m.opcode = OC_BR_PUBLISH;
			SOCKET_S(rmd.s3, mom_message_t, m);
		}
		else if(m.opcode == OC_BR_PUBLISH) {
			// If it is mine, end here
			if(m.global_id == -rmd.broker_id)
				continue;
			// If not, forward to next broker instance and DBMS
			SOCKET_S(rmd.s3, mom_message_t, m);
			msq_send(rmd.msqid_dbms, &m, sizeof(mom_message_t));
		}
		else {
			printf("%d: WARNING: Ring master received non-publish message of opcode %d\n", getpid(), m.opcode);
			continue;
		}
		
	}

	printf("%d: Closing ring master...\n", getpid());
	socket_destroy(rmd.s3);
	
	return 0;
}
