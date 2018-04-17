#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include "mom_general.h"
#include "libs/msq.h"
#include "libs/argv_parser.h"

int main(int argc, char* argv[]){
	ap_t* ap = ap_create_from_argv(argc, argv);
	if(!ap) {
		printf("%d: Error creating argv_parser:%d\n", getpid(), errno);
		exit(-1);
	}
	
	int msqid, msq_sockets;
	ap_get_int(ap, QUEUE_REQUESTER, &msqid);
	ap_get_int(ap, "socket", &msq_sockets);
	
	printf("Daemon requester is up!\n");

	// Requester main loop
	while(1) {
		mom_message_t m = {0};
		msq_rcv(msqid, &m, sizeof(mom_message_t), 0);
		printf("Received a message from user!\n");
		print_message(m);
		// TODO: Forward via socket
		msq_send(msq_sockets, &m, sizeof(mom_message_t));
	}
	
	ap_destroy(ap);
	exit(0);
}
