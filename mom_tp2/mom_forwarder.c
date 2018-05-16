#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

#include "mom_general.h"
#include "libs/msq.h"
#include "libs/socket.h"
#include "libs/argv_parser.h"

#define ATTEMPTS_RECEIVING 10

#define FORWARDER_FILE_FORMAT "fdb_%ld.dat"
#define ONWAIT_FILE "fdbw.dat"
#define FORWARDER_FILE_TMP "fdbwtmp.dat"
#define GID_SIZE 10
#define FDBS_LINE (PAYLOAD_SIZE + 2)

/* Creates a temporal file with a given file_name. */
void create_fdb_file(long global_id){
	char file_name[PATH_MAX] = {0};
	sprintf(file_name, FORWARDER_FILE_FORMAT, global_id);
	
	if(mknod(file_name, S_IFREG|0666, 0) < 0) {
			// In case they already exist, I ain't care
			if(errno == EEXIST)	return;

			printf("%d: Error creating file %s: %d\n", getpid(), file_name, errno);
			exit(-1);
		}
}

bool write_payload(FILE* fp, char* payload) {
	char aux[FDBS_LINE] = {0};
	sprintf(aux, "%s", payload);
	
	return (fwrite((void*) aux, sizeof(aux), 1, fp) > 0);
}

int read_payload(FILE* fp, char* payload) {
	char aux[FDBS_LINE] = {0};
	int r = fread((void*) aux, sizeof(aux), 1, fp);
	if(r > 0)
		sprintf(payload, "%s", aux);

	return r;
}

int retrieve_next_payload(long global_id, char* payload) {
	char file_name[PATH_MAX] = {0};
	sprintf(file_name, FORWARDER_FILE_FORMAT, global_id);
		
	FILE* fp = fopen(file_name, "ab+");
	if(!fp) {
		printf("%d: Error opening file %s: %d", getpid(), file_name, errno);
		exit(-1);
	}
	
	FILE* fp_tmp = fopen(FORWARDER_FILE_TMP, "wb+");
	if(!fp_tmp) {
		printf("%d: Error opening file %s: %d", getpid(), FORWARDER_FILE_TMP, errno);
		exit(-1);
	}
	
	int r = read_payload(fp, payload);
	
	// Copy rest of file to tmp
	char aux[FDBS_LINE] = {0};
	while(fread(&aux, sizeof(aux), 1, fp) > 0)
			fwrite(&aux, sizeof(aux), 1, fp_tmp);

	fclose(fp);
	fclose(fp_tmp);

	remove(file_name);
	rename(FORWARDER_FILE_TMP, file_name);
	
	return r;
}

void store_message(mom_message_t* m){
	char file_name[PATH_MAX] = {0};
	sprintf(file_name, FORWARDER_FILE_FORMAT, m->mtype);
	
	FILE* fp = fopen(file_name, "ab+");
	if(!fp) {
		printf("%d: Error opening file %s: %d", getpid(), file_name, errno);
		exit(-1);
	}
	
	if(!write_payload(fp, m->payload)) {
		printf("%d: Error writing payload to file %s: %d", getpid(), file_name, errno);
		exit(-1);		
	}
	
	fclose(fp);
}

void make_gid_wait(long global_id){
	FILE* fp = fopen(ONWAIT_FILE, "ab+");
	if(!fp) {
		printf("%d: Error opening file %s: %d", getpid(), ONWAIT_FILE, errno);
		exit(-1);
	}
	
	char gid[GID_SIZE] = {0};
	sprintf(gid, "%ld", global_id);
	
	fwrite(&gid, sizeof(gid), 1, fp);
	
	fclose(fp);
}

bool gid_is_waiting(long global_id) {
	FILE* fp = fopen(ONWAIT_FILE, "ab+");
	if(!fp) {
		printf("%d: Error opening file %s: %d", getpid(), ONWAIT_FILE, errno);
		return false;
	}
	FILE* fp_tmp = fopen(FORWARDER_FILE_TMP, "wb+");
	if(!fp_tmp) {
		printf("%d: Error opening file %s: %d", getpid(), FORWARDER_FILE_TMP, errno);
		return false;
	}
	char gid[GID_SIZE] = {0};
	char desired_gid[GID_SIZE] = {0};
	sprintf(desired_gid, "%ld", global_id);
	
	bool found = false;
	
	while(fread(&gid, sizeof(gid), 1, fp) > 0) {
		if(strcmp (gid, desired_gid) == 0)
			found = true;
		else
			fwrite(&gid, sizeof(gid), 1, fp_tmp);
	}

	fclose(fp);
	fclose(fp_tmp);

	remove(ONWAIT_FILE);
	rename(FORWARDER_FILE_TMP, ONWAIT_FILE);
	return found;
}

bool keep_looping = true;

void handler(int signum) {
  keep_looping = false;
}

void set_handler() {
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = handler;
	sigaction(SIGINT, &sa, NULL);
}

int main(int argc, char* argv[]){
	set_handler();
	ap_t* ap = ap_create_from_argv(argc, argv);
	if(!ap) {
		printf("%d: Error creating argv_parser: %d\n", getpid(), errno);
		exit(-1);
	}
	
	int msqid_resp, msq_forwarder;
	ap_get_int(ap, QUEUE_RESPONSER, &msqid_resp);
	ap_get_int(ap, QUEUE_FORWARDER, &msq_forwarder);
	
	
	printf("Daemon forwarder is up! (PID: %d)\n", getpid());

	// Forwarder main loop
	while(keep_looping) {
		mom_message_t m = {0};
		msq_rcv(msq_forwarder, &m, sizeof(mom_message_t), 0);
		if(!keep_looping) break;
		
		if(m.opcode == OC_DELIVERED) {
			if(gid_is_waiting(m.mtype))
				msq_send(msqid_resp, &m, sizeof(mom_message_t));
			else
				store_message(&m);
		}
		else if(m.opcode == OC_RECEIVE) {
			char payload[PAYLOAD_SIZE] = {0};
			if(retrieve_next_payload(m.global_id, payload) > 0) {
				sprintf(m.payload, "%s", payload);
				m.mtype = m.global_id;
				m.opcode = OC_DELIVERED;
				msq_send(msqid_resp, &m, sizeof(mom_message_t));
			}
			else
				make_gid_wait(m.global_id);
		}
		else
			printf("%d: Error on forwarder, message received invalid: %d\n", getpid(), m.opcode);
		
	}
	
	// If here, must close mom_forwarder. Make the broker
	
	ap_destroy(ap);
	printf("\nClosing mom_forwarder...\n");

	exit(0);
}
