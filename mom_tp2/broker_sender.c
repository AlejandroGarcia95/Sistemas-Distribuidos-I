#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

#include "mom_general.h"
#include "libs/msq.h"
#include "libs/socket.h"
#include "libs/argv_parser.h"


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

int main(int argc, char* argv[]) {
	ap_t* ap = ap_create_from_argv(argc, argv);
	if(!ap) {
		printf("%d: Error creating argv_parser:%d\n", getpid(), errno);
		exit(-1);
	}
	
	int socket_fd, msqid;
	ap_get_int(ap, SOCKET_FD, &socket_fd);
	ap_get_int(ap, QUEUE_SENDER, &msqid);
	
	socket_t* s = socket_create_from_fd(socket_fd, SOCK_ACTIVE);
	if(!s) {
		printf("%d: Error creating socket on handler: %d\n", getpid(), errno);
		exit(-1);
	}
	
	set_handler();
	printf("A broker sender is up (PID: %d) !\n", getpid());
	
	long this_mtype = getpid();
	
	// Sender main loop
	while(keep_looping) {
		mom_message_t m = {0};
		msq_rcv(msqid, &m, sizeof(mom_message_t), this_mtype);
		if(!keep_looping)	break;
		printf("%ld: A sender is delivering a message to a machine!\n", this_mtype);
		print_message(m);
		SOCKET_S(s, mom_message_t, m);	
	}
	
	// If here, tell the mom_daemon on the user's machine
	// to panic since broker won't be available any longer
	mom_message_t m = {0};
	m.opcode = OC_SEPPUKU;
	SOCKET_S(s, mom_message_t, m);
	
	printf("\nClosing a broker sender (PID: %d)...\n", getpid());
	socket_destroy(s);
	ap_destroy(ap);
	exit(0);
}
