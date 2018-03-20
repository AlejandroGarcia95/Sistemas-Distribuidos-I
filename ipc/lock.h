#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>

//------------------------------ LOCK USE ------------------------------ 
/*

         Any process
              |
 lock_create  |
              |
 Can use      |
 lock_acquire |
 lock_release |
 as desired   |
              |
 lock_destroy |
              |

*/

int lock_create(char* file){
	int fd = open(file, O_CREAT|O_WRONLY, 0777);
	if(fd < 0){
		printf("%d: Error opening file %s for lock: %d\n", getpid(), file, errno);
		exit(-1);
	}
	return fd;
}

void lock_destroy(int fd){
	if(close(fd) < 0){
		printf("%d: Error destroying lock: %d\n", getpid(), errno);
		exit(-1);
	}	
}

void lock_acquire(int fd, bool is_exclusive){
	struct flock fl;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_type = (is_exclusive ? F_WRLCK : F_RDLCK);
	if(fcntl(fd, F_SETLKW, &fl) < 0){
		printf("%d: Error acquiring lock: %d\n", getpid(), errno);
		exit(-1);
	}
}

void lock_release(int fd){
	struct flock fl;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_type = F_UNLCK;
	if(fcntl(fd, F_SETLKW, &fl) < 0){
		printf("%d: Error releasing lock: %d\n", getpid(), errno);
		exit(-1);
	}
}
