#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "argv_parser.h"


/* Creates an ep which will be used to exec into binary_name.
 * Initializes first string in the parameter list with the
 * name providen. Returns NULL on failure.*/
ap_t* ap_create(char* binary_name){
	if(!binary_name) return NULL;
	
	ap_t* ap = (ap_t*) malloc(sizeof(ap_t));
	if(!ap)	return NULL;
	// Allocates first parameter in the list
	char* parm0 = (char*) malloc(sizeof(char) * PARM_MAX_LEN);
	if(!parm0) {
		free(ap);
		return NULL;
	}
	
	sprintf(parm0, "%s", binary_name);
	ap->parm_list[0] = parm0;
	ap->parm_amount = 1;
	
	return ap;
}

/* Destroys the received ep and its parameters list.*/
void ap_destroy(ap_t* ap){
	if(!ap)	return;
	int i;
	if(ap->parm_list)
		for(i = 0; i < ap->parm_amount; i++)
			free(ap->parm_list[i]);
	free(ap);
}

/* Adds an int value to the current ep parameter list with name
 * parm_name. Returns true if successful, false otherwise.*/
bool ap_set_int(ap_t* ap, char* parm_name, int value){
	char buff[PARM_MAX_LEN];
	sprintf(buff, "%d", value);
	return ap_set_string(ap, parm_name, buff);
}

/* Adds a double value to the current ep parameter list with name
 * parm_name. Returns true if successful, false otherwise.*/
bool ap_set_double(ap_t* ap, char* parm_name, double value){
	char buff[PARM_MAX_LEN];
	sprintf(buff, "%lf", value);
	return ap_set_string(ap, parm_name, buff);
}

/* Adds a string value to the current ep parameter list with name
 * parm_name. Returns true if successful, false otherwise.*/
bool ap_set_string(ap_t* ap, char* parm_name, char* value){
	if((!parm_name) || (!value))	return 0;
	// Allocates parameter name and value
	char* p_name = (char*) malloc(sizeof(char) * strlen(parm_name) + 1);
	if(!p_name) 
		return 0;
	
	char* p_value= (char*) malloc(sizeof(char) * strlen(value) + 1);
	if(!p_value){
		free(p_name);
		return 0;
	}
	
	// Copies parameter name and value into allocated buffers
	sprintf(p_name, "%s", parm_name);
	sprintf(p_value, "%s", value);
	ap->parm_list[ap->parm_amount] = p_name;
	ap->parm_list[ap->parm_amount + 1] = p_value;
	ap->parm_amount += 2;
	return 1;
}

/* Wrapper for exec. The binary to be called with exec is the
 * same as provided when creating the ep.*/
int ap_exec(ap_t* ap){
	if((!ap) || (!ap->parm_list) || (ap->parm_amount == 0))
		return -1;
	return execv(ap->parm_list[0], ap->parm_list);
}

/* Creates an ep from an already made argv list of parameters.*/
ap_t* ap_create_from_argv(int argc, char* argv[]){
	if(!argv)	return NULL;
	if(!argv[0])	return NULL;
	ap_t* ap = ap_create(argv[0]);
	if(!ap)	return NULL;
	// Adds every argv string to parm_list
	int i;
	for(i = 1; i < argc; i += 2) {
		if(!ap_set_string(ap, argv[i], argv[i+1])){
			ap_destroy(ap);
			return NULL;
		}
	}
		
	return ap;
}

/* Recovers an int value with name parm_name from the curret ep 
 * parameter list, and stores it in *value. Returns true if
 * successful, false otherwise.*/
bool ap_get_int(ap_t* ap, char* parm_name, int* value){
	char buf[PARM_MAX_LEN];
	if(ap_get_string(ap, parm_name, buf)){
		sscanf(buf, "%d", value);
		return 1;
	}
	return 0;
}

/* Recovers a double value with name parm_name from the curret ep 
 * parameter list, and stores it in *value. Returns true if
 * successful, false otherwise.*/
bool ap_get_double(ap_t* ap, char* parm_name, double* value){
	char buf[PARM_MAX_LEN];
	if(ap_get_string(ap, parm_name, buf)){
		sscanf(buf, "%lf", value);
		return 1;
	}
	return 0;
}

/* Recovers a string value with name parm_name from the curret ep 
 * parameter list, and stores it in *value. Returns true if
 * successful, false otherwise.*/
bool ap_get_string(ap_t* ap, char* parm_name, char* value){
	if((!parm_name) || (!value))	return 0;
	// Linearly search parm_list
	int i;
	for(i = 1; i < ap->parm_amount; i += 2){
		if(strcmp(parm_name, ap->parm_list[i]) == 0){
			sprintf(value, "%s", ap->parm_list[i + 1]);
			return 1;
		}
	}
	
	return 0;
}

/* Creates a copy of the received ap */
ap_t* ap_clone(ap_t* ap){
	return ap_create_from_argv(ap->parm_amount, ap->parm_list);
}
