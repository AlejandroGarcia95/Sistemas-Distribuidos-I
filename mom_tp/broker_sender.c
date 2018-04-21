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
#include "libs/argv_parser.h"




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
	
	printf("A broker sender is up (PID: %d) !\n", getpid());
	
	long this_mtype = getpid();
	
	// Sender main loop
	while(1) {
		mom_message_t m = {0};
		msq_rcv(msqid, &m, sizeof(mom_message_t), 0);
		printf("%d: A sender is delivering a message to a machine!\n", getpid());
		print_message(m);
		this_mtype = m.global_id;
		SOCKET_S(s, mom_message_t, m);	
	}
	
	socket_destroy(s);
	ap_destroy(ap);
	exit(0);
}
