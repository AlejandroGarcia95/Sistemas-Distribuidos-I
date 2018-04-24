#ifndef _MSG_H_
#define _MSG_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#define MSQ_MAGIC_INT 11

//------------------------------ MSQ USE ------------------------------- 
/*

			Parent process (initializer/destroyer)
            |
 msq_create |
            |
            | [fork] 
            |	\
            |	 \
            |	  \  [exec] Give child process
            |	   \ the msqid as argument
            |	    \  
 can use    |	     \
 msq_send or|	      \
 msq_rcv as |	       \
 desired    |          Child process
            |          |
            |          | Can use msq_send and
            |          | msq_rcv as desired
            |          |
            |        [exit] 
            |-----------
msq_destroy |            
            |            

*/

int msq_create(char* file){
	key_t key = ftok(file, MSQ_MAGIC_INT);
	if(key < 0) {
		printf("%d: Error creating message queue: %d\n", getpid(), errno);
		exit(-1);
	} 
	int msqid = msgget(key, IPC_CREAT|0644);
	if(msqid < 0){
		printf("%d: Error creating message queue: %d\n", getpid(), errno);
		exit(-1);
	}
	return msqid;
}

void msq_send(int msqid, const void *msg, size_t msgsz){
    if(msgsnd(msqid, msg, msgsz - sizeof(long), 0) == -1){
        printf("%d: Error sending message  on queue %d: %d\n", getpid(), msqid, errno);
        exit(-1);
    }
}

void msq_rcv(int msqid, void *msg, size_t msgsz, long type){
    if(msgrcv(msqid, msg, msgsz - sizeof(long), type, 0) == -1){
        if(errno == EINTR)	// If signal arrived...
			return;
		printf("%d: Error receiving message  on queue %d: %d\n", getpid(), msqid, errno);
        exit(-1);
    }
}

void msq_destroy(int msqid){
	if(msgctl(msqid, IPC_RMID, 0) < 0){
		printf("%d: Error destroying message queue %d: %d\n", getpid(), msqid, errno);
		exit(-1);
	}
}

#endif /* _MSG_H_ */
