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
	ap_get_int(ap, QUEUE_RESPONSER, &msqid);
	ap_get_int(ap, "socket", &msq_sockets);
	
	printf("Daemon responser is up!\n");
	// Responser main loop
	while(1) {
		mom_message_t m = {0};
		// TODO: Receive from socket
		msq_rcv(msq_sockets, &m, sizeof(mom_message_t), 0);
		printf("Received a message from broker!\n");
		print_message(m);
		msq_send(msqid, &m, sizeof(mom_message_t));
	}
	
	ap_destroy(ap);
	exit(0);
}
