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

/* Handle every situation like a dog: if you can't eat it or 
 play with it, just pee on it and walk away. */

#define ATTEMPTS_RECEIVING 10


pid_t launch_sender(ap_t* ap_sender) {
	pid_t pid = fork();
	if(pid < 0) {
		printf("%d: Error launching sender: %d\n", getpid(), errno);
		return -1;
	}
	if(pid == 0) {
		// If here, this is the sender
		ap_exec(ap_sender);
	}
	
	return pid;
}


ap_t* create_sender_ap(int socket_fd, int msqid_h, int msqid_s) {
	ap_t* ap = ap_create("./broker_sender");
	if(!ap) {
		printf("%d: Error creating argv_parser:%d\n", getpid(), errno);
		exit(-1);
	}
	
	ap_set_int(ap, SOCKET_FD, socket_fd);
	ap_set_int(ap, QUEUE_HANDLER, msqid_h);
	ap_set_int(ap, QUEUE_SENDER, msqid_s);
	return ap;
}

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
	
	int socket_fd, msqid_h, msqid_s;
	ap_get_int(ap, SOCKET_FD, &socket_fd);
	ap_get_int(ap, QUEUE_HANDLER, &msqid_h);
	ap_get_int(ap, QUEUE_SENDER, &msqid_s);
	
	ap_t* ap_sender = create_sender_ap(socket_fd, msqid_h, msqid_s);
	pid_t s_pid = launch_sender(ap_sender);
	ap_destroy(ap_sender);
	
	socket_t* s = socket_create_from_fd(socket_fd, SOCK_ACTIVE);
	if(!s) {
		printf("%d: Error creating socket on handler: %d\n", getpid(), errno);
		exit(-1);
	}
	
	set_handler();
	printf("A broker handler is up (PID: %d) !\n", getpid());
	int att_rcv = 0;
	// Handler main loop
	while(keep_looping) {
		mom_message_t m = {0};
		SOCKET_R(s, mom_message_t, m);
		if(!keep_looping)	break;
		if((m.opcode == OC_SEPPUKU) || (att_rcv == ATTEMPTS_RECEIVING)) {
			// If here, the other machine has killed daemon. This
			// handler and sender services are no longer needed
			kill(s_pid, SIGINT);
			break;
		}
		if(((int) m.opcode) == 0) { // No message received
			printf("WARNING: No message received from machine\n");
			att_rcv++;
			continue;
		}
		att_rcv = 0;
		printf("%d: A handler has received a message from a machine!\n", getpid());
		print_message(m);
		m.mtype = s_pid;
		// Forward message to DBMS
		msq_send(msqid_h, &m, sizeof(mom_message_t));
	}
	
	printf("\nClosing a broker handler (PID: %d)...\n", getpid());
	socket_destroy(s);
	ap_destroy(ap);
	exit(0);
}
