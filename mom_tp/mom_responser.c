#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include "mom_general.h"
#include "libs/msq.h"
#include "libs/socket.h"
#include "libs/argv_parser.h"

#define ATTEMPTS_RECEIVING 10

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

int main(int argc, char* argv[]){
	set_handler();
	ap_t* ap = ap_create_from_argv(argc, argv);
	if(!ap) {
		printf("%d: Error creating argv_parser:%d\n", getpid(), errno);
		exit(-1);
	}
	
	int msqid, socket_fd;
	ap_get_int(ap, QUEUE_RESPONSER, &msqid);	
	ap_get_int(ap, SOCKET_FD, &socket_fd);
	
	socket_t* s = socket_create_from_fd(socket_fd, SOCK_ACTIVE);
	if(!s) {
		printf("%d: Error creating socket on responser: %d\n", getpid(), errno);
		exit(-1);
	}
	
	printf("Daemon responser is up! (PID: %d)\n", getpid());
	int att_rcv = 0;
	// Responser main loop
	while(keep_looping) {
		mom_message_t m = {0};
		// Receive from socket
		SOCKET_R(s, mom_message_t, m);
		if(!keep_looping) break;
		if((m.opcode == OC_SEPPUKU) || (att_rcv == ATTEMPTS_RECEIVING)) {
			// If here, the broker has died and we should all panic
			printf("\n-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o\n\n");
			printf("CRITICAL: THE BROKER IS DOWN!\n");
			printf("Closing mom daemon, try again later\n");
			printf("\n-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o-o\n");
			kill(0, SIGINT);
			break;
		}
		if(((int) m.opcode) == 0) { // No message received
			printf("WARNING: No response from broker\n");
			att_rcv++;
			continue;
		}
		att_rcv = 0;
		printf("Responser received a message from broker!\n");
		print_message(m);
		m.mtype = m.local_id;
		msq_send(msqid, &m, sizeof(mom_message_t));
	}
	
	printf("\nClosing mom_responser...\n");
	socket_destroy(s);
	ap_destroy(ap);
	exit(0);
}
