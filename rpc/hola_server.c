/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include "hola.h"
#include <stdio.h>

int *
print_hola_1_svc(void *argp, struct svc_req *rqstp)
{
	static int  result;

	/*
	 * insert server code here
	 */

	pritnf("Hello RPC World!!!\n");
	result++;

	return &result;
}