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

int main(int argc, char* argv[]){
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
	
	printf("Daemon responser is up!\n");
	// Responser main loop
	while(1) {
		mom_message_t m = {0};
		// Receive from socket
		SOCKET_R(s, mom_message_t, m);
		printf("Responser received a message from broker!\n");
		print_message(m);
		m.mtype = m.local_id;
		msq_send(msqid, &m, sizeof(mom_message_t));
	}
	
	socket_destroy(s);
	ap_destroy(ap);
	exit(0);
}
