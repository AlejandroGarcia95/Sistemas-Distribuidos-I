#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include "mom_general.h"
#include "libs/msq.h"

/* We all need to be mocked from time to time, 
 * lest we take ourselves too seriously. */

int main(int argc, char* argv[]){
	
	int msq_socket_from_machine = msq_create("mom_requester.c"); // To mock socket !!
	int msq_socket_to_machine = msq_create("mom_responser.c"); // To mock socket !!
	
	printf("Broker mock is up!\n");
	
	while(1){
		mom_message_t m = {0};
		msq_rcv(msq_socket_from_machine, &m, sizeof(mom_message_t), 0);
		printf("Received a message from a machine!\n");
		print_message(m);
		// TODO: Process message
		m.opcode = OC_ACK_SUCCESS;
		msq_send(msq_socket_to_machine, &m, sizeof(mom_message_t));
	}
	
	return 0;
}
