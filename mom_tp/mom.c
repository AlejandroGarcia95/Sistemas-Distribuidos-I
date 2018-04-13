#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "mom.h"
#include "libs/msq.h"

mom_t* mom_create(){
	int p = msq_create("mom.h");
	msq_destroy(p);
	return NULL;
}
