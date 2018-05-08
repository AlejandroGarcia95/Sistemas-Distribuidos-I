#ifndef ap_H
#define ap_H
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define PARM_MAX_LEN 50
#define PARM_MAX_AMOUNT 100 

// If using this, DO NOT CALL EXEC!!! Insted, use ap_exec below!

typedef struct ap_ {
	char* parm_list[PARM_MAX_AMOUNT * 2 + 1];
	size_t parm_amount;
} ap_t;

/* Destroys the received ep and its parameters list.*/
void ap_destroy(ap_t* ap);

// To be used for SENDING PARAMETERS

/* Creates an ep which will be used to exec into binary_name.
 * Initializes first string in the parameter list with the
 * name providen. Returns NULL on failure.*/
ap_t* ap_create(char* binary_name);

/* Adds an int value to the current ep parameter list with name
 * parm_name. Returns true if successful, false otherwise.*/
bool ap_set_int(ap_t* ap, char* parm_name, int value);

/* Adds a double value to the current ep parameter list with name
 * parm_name. Returns true if successful, false otherwise.*/
bool ap_set_double(ap_t* ap, char* parm_name, double value);

/* Adds a string value to the current ep parameter list with name
 * parm_name. Returns true if successful, false otherwise.*/
bool ap_set_string(ap_t* ap, char* parm_name, char* value);

/* Wrapper for exec. The binary to be called with exec is the
 * same as provided when creating the ep.*/
int ap_exec(ap_t* ap);

// To be used for RECEIVING PARAMETERS

/* Creates an ep from an already made argv list of parameters.*/
ap_t* ap_create_from_argv(int argc, char* argv[]);

/* Recovers an int value with name parm_name from the curret ep 
 * parameter list, and stores it in *value. Returns true if
 * successful, false otherwise.*/
bool ap_get_int(ap_t* ap, char* parm_name, int* value);

/* Recovers a double value with name parm_name from the curret ep 
 * parameter list, and stores it in *value. Returns true if
 * successful, false otherwise.*/
bool ap_get_double(ap_t* ap, char* parm_name, double* value);

/* Recovers a string value with name parm_name from the curret ep 
 * parameter list, and stores it in *value. Returns true if
 * successful, false otherwise.*/
bool ap_get_string(ap_t* ap, char* parm_name, char* value);

/* Creates a copy of the received ap */
ap_t* ap_clone(ap_t* ap);

#endif
