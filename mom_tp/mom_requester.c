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
		printf("%d: Error creating argv_parser: %d\n", getpid(), errno);
		exit(-1);
	}
	
	int msqid, socket_fd;
	ap_get_int(ap, QUEUE_REQUESTER, &msqid);
	ap_get_int(ap, SOCKET_FD, &socket_fd);
	
	socket_t* s = socket_create_from_fd(socket_fd, SOCK_ACTIVE);
	if(!s) {
		printf("%d: Error creating socket on requester: %d\n", getpid(), errno);
		exit(-1);
	}
	
	printf("Daemon requester is up! (PID: %d)\n", getpid());

	// Requester main loop
	while(keep_looping) {
		mom_message_t m = {0};
		msq_rcv(msqid, &m, sizeof(mom_message_t), 0);
		if(!keep_looping) break;
		printf("Requester received a message from user!\n");
		print_message(m);
		// Forward via socket
		SOCKET_S(s, mom_message_t, m);
	}
	
	// If here, must close mom_requester. Make the broker
	// handler and sender die as well, since they ain't needed
	mom_message_t m = {0};
	m.opcode = OC_SEPPUKU;
	SOCKET_S(s, mom_message_t, m);
	
	socket_destroy(s);
	ap_destroy(ap);
	printf("\nClosing mom_requester...\n");
	exit(0);
}
