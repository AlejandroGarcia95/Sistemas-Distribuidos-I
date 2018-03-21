#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "msq.h"

#define MSG_AMOUNT 50
#define CHILDREN_LOOP 5
#define CHILDREN_AMOUNT (MSG_AMOUNT/CHILDREN_LOOP)

struct msg{
	long m_type;
	int data;	
};

void children_main(int msqid){
	// First sleep a little
	usleep(1000);
	int i;
	struct msg m;
	// Loop while trying to receive message
	for(i = 0; i < CHILDREN_LOOP; i++) {
		msq_rcv(msqid, (void*) &m, sizeof(struct msg), i + 1);
		printf("I am child %d and I received a message (mtype: %ld, data: %d)\n", getpid(), m.m_type, m.data);
		usleep(1000);
	}
	
	exit(0);
}


int main(int arg, char* argv[]){
	// Create and initialize msq
	int msqid = msq_create("msq_test.c");
	int i;
	
	// Launch children to wait msq
	for(i = 0; i < CHILDREN_AMOUNT; i++) {
		pid_t pid = fork();
		if(pid < 0) {
			printf("%d: Error launching child: %d\n", getpid(), errno);
			exit(-1);
		}
		if(pid == 0)
			children_main(msqid);
	}
	// Send MSG_AMOUNT messages via msq
	for(i = 0; i < MSG_AMOUNT; i++) {
		// data is the number of message from 1 to MSG_AMOUN
		struct msg m = {i/CHILDREN_AMOUNT + 1, i + 1};
		msq_send(msqid, (void*) &m, sizeof(struct msg));
	}
		
	// Wait for children
	for(i = 0; i < CHILDREN_AMOUNT; i++)
		wait(NULL);
		
	 // Destroy msq
	 msq_destroy(msqid);
			
	return 0;
}
