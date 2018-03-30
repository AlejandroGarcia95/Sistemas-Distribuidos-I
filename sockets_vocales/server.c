#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


void remove_vowels(char* str) {
	int j = 0;
	for (int i = 0; i < 100; i++) {
		char c = str[i];
		if (c != 'a' && c != 'A' &&
		        c != 'e' && c != 'E' &&
		        c != 'i' && c != 'I' &&
		        c != 'o' && c != 'O' &&
		        c != 'u' && c != 'U') {
			str[j] = c;
			j++;
		}
	}
	if (j < 100) {
		str[j] = '\0';
	}
}


int main(int argc, char* argv[]) {

	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in myaddr;
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(8080);
	myaddr.sin_addr.s_addr = INADDR_ANY;


	socklen_t myaddr_size = sizeof(myaddr);

	if (bind(socket_fd, (struct sockaddr*) &myaddr, myaddr_size ) < 0) {
		perror("Error en bind");
		exit(-1);
	}

	if (listen(socket_fd, 10) < 0) {
		perror("Error en listen");
		exit(-1);
	}

	struct sockaddr_in client_addr;

	socklen_t client_addr_size = sizeof(client_addr);

	while (1) {

		printf("Esperando conexiones...\n");
		int client_fd = accept(socket_fd, (struct sockaddr*) &client_addr, &client_addr_size);

		if (client_fd < 0) {
			perror("Error en accept");
			exit(-1);
		}


		char buffer[100] = {0};
		if (read(client_fd, buffer, sizeof(buffer) / sizeof(char)) < 0) {
			perror("Error en read.");
		} else {
			printf("Llego: %s\n", buffer);
			remove_vowels(buffer);
			printf("Respondiendo: %s\n", buffer);
			if (write(client_fd, buffer, sizeof(buffer) / sizeof(char)) < 0) {
				perror("Error en write");
				exit(-1);
			}
		}
	}
	printf("Fin server\n");
}
