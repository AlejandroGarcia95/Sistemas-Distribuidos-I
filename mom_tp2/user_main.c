#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

#include "mom.h"

#define BUFF_SIZE 200


void print_help(){
	printf("USAGE:\n\n");
	printf("Subscribing: S <topic>\n");
	printf("Unsubscribing: U <topic>\n");
	printf("Publishing: P <topic> <text>\n");
	printf("Receiving: R\n\n");
	printf("Exit program: E\n\n");
}

bool parse_command(mom_t* mom, char* buff){
	int i;
	if((buff[1] != ' ') && (buff[1] != '\0')) {
		printf("Damn you user! Use the program properly!\n");
		return false;
	}
	char l = buff[0];
	switch(l){
		case 's':
		case 'S':
			printf("Subscribing to topic %s...\n", &buff[2]);
			if(mom_subscribe(mom, &buff[2]))
				printf("Subscribing successful!\n");
			else
				printf("Subscribing failed!\n");
			return true;
			
		case 'u':
		case 'U':
			printf("Unsubscribing from topic %s...\n", &buff[2]);
			if(mom_unsubscribe(mom, &buff[2]))
				printf("Unsubscribing successful!\n");
			else
				printf("Unsubscribing failed!\n");
			return true;
			
		case 'r':
		case 'R':
			printf("Receiving from mom...\n");
			char msg[BUFF_SIZE];
			if(mom_receive(mom, &msg))
				printf("Receiving successful! Message received is: \n%s\n", msg);
			else
				printf("Receiving failed!\n");
			return true;
			
		case 'p':
		case 'P':
			for(i = 2; i < BUFF_SIZE && buff[i] != ' '; i++) ;
			if(i >= BUFF_SIZE){
				printf("Damn you user! Use the program properly!\n");
				return false;
			}
			char topic[BUFF_SIZE];
			strncpy(topic, &buff[2], i - 2);
			topic[i-2] = '\0';
			printf("Publishing to topic %s the text %s...\n", topic, &buff[i+1]);
			if(mom_publish(mom, topic, (const char*) &buff[i+1]))
				printf("Publishing successful!\n");
			else
				printf("Publishing failed!\n");
			return true;
			
		case 'e':
		case 'E':
			printf("Now exiting program\n");
			return false;
			
		default:
			printf("Damn you user! Use the program properly!\n");
			return false;
	}
}

int main(int argc, char* argv[]){
	printf("User process instance %d has been launched!\n", getpid());
	mom_t* mom = mom_create();
	if(!mom) {
		printf("Something went wrong while creating mom\n");
		return -1;
	} 

	print_help();
	bool go_on = true;
	
	while(go_on) {
		char buff[BUFF_SIZE] = {0};
		fgets(buff, BUFF_SIZE - 1, stdin);
		buff[strlen(buff) - 1] = '\0'; // Replace newline with null
		go_on = parse_command(mom, buff);
	}

	mom_destroy(mom);
	return 0;
}
