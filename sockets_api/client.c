#define _POSIX_C_SOURCE 200112L

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>	// memset
#include <stdlib.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>

#include "socket.h"

#define BUFF_SIZE 234

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT "8080"

void read_buffer(char* buff){
	fgets(buff, BUFF_SIZE, stdin);
	size_t last = strlen(buff);
	if(buff[last-1] == '\n')
		buff[last-1] = '\0';
}


int main(int argc, char* argv[]){
	
	socket_t* s = socket_create(SOCK_ACTIVE);
	if(!s){
		printf("%d:Error creating socket", getpid());
		exit(-1); 
	}
	
	socket_conect(s, SERVER_IP, SERVER_PORT);
	printf("-- Connection established --\n");
	
	int chat = 1;
	while(chat){
		char buff[BUFF_SIZE] = {0};
		read_buffer(buff);
		socket_send(s, buff, BUFF_SIZE);
		if(strcmp(buff, "CHAU") == 0)
			chat = 0;
		else {
			socket_receive(s, buff, BUFF_SIZE);
			printf("Server: %s\n", buff);
			if(strcmp(buff, "CHAU") == 0)
				chat = 0;
		}
	}
	
	
	socket_destroy(s);
	return 0;
}
