#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include "mom_general.h"
#include "libs/msq.h"
#include "libs/socket.h"

/* We all need to be mocked from time to time, 
 * lest we take ourselves too seriously. */
 
bool keep_looping = true;

void handler(int signum) {
  printf("Closing broker mock...\n");
  keep_looping = false;
}

void set_handler() {
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = handler;
	sigaction(SIGINT, &sa, NULL);
}

int main(int argc, char* argv[]){
	// Set handler for graceful quit
	set_handler();
	// Create broker side socket
	socket_t* s = socket_create(SOCK_PASSIVE);
	if(!s){
		printf("%d: Error creating socket", getpid());
		exit(-1); 
	}
	
	socket_bind(s, SERVER_IP, SERVER_PORT);
	socket_listen(s, 0);
	keep_looping = true;
	printf("Broker mock is up!\n");
	
	socket_t* s2 = socket_accept(s);
	
	while(keep_looping){
		mom_message_t m = {0};
		SOCKET_R(s2, mom_message_t, m);
		if(!keep_looping)	break;
		printf("Received a message from a machine!\n");
		print_message(m);
		// TODO: Process message
		m.opcode = OC_ACK_SUCCESS;
		SOCKET_S(s2, mom_message_t, m);
	}
	
	socket_destroy(s2);
	socket_destroy(s);
	
	return 0;
}
