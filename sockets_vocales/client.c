#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 500

#include "socket.h"
#include <string.h>

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

//#define SERVER_IP "157.92.50.254"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT "8080"

int main(int argc, char* argv[]) {
	printf("Creating socket\n");
	socket_t* s = socket_create(SOCK_ACTIVE);
	if (!s) {
		printf("Error creating socket\n");
		return -1;
	}

	printf("Connecting socket\n");
	int err = socket_conect(s, SERVER_IP, SERVER_PORT);
	if (err < 0) {
		printf("Error connecting\n");
		socket_destroy(s);
		return -1;
	}


	err = socket_send(s, argv[1], strlen(argv[1] + 1));
	if (err < 0) {
		printf("Error sending message\n");
		socket_destroy(s);
		return -1;
	}

	char buff[1000];
	err = socket_receive(s, (char*) &buff, strlen(argv[1]));
	if (err < 0) {
		printf("Error receiving message\n");
		socket_destroy(s);
		return -1;
	}

	printf("Destroying socket\n");
	socket_destroy(s);

	printf("%s\n", buff);

	return 0;
}
