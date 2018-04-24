#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "mom_general.h"
#include "dbfs.h"
#include "libs/msq.h"
#include "libs/argv_parser.h"

/* Reality is merely an illusion, albeit a very persistent one.*/

// ---------- General info struct for the whole DBMS ----------

typedef  struct dbms_data_ {
	long global_id;
	int msqid_h;
	int msqid_s;

	int inv_index;
	
} dbms_data_t;



// ---------------- Opcode treatment functions ----------------

/* Checks if user has ever registered before. Returns true if the
 user's really registered, false if not. */
bool user_is_registered(mom_message_t* m, long global_id, bool print_warning) { 
	if(m->global_id < global_id)
		return true;
	if(print_warning)
		printf("%d: Warning! Some not registered user was trying to make a query!\n", getpid());
	return false;
}

/* Registers a new user, setting the mom_message global_id field
 to the proper global_id of the sender. Sets the message opcode 
 to the proper value depending on success. */
void register_user(mom_message_t* m, dbms_data_t* dd) {
	if(user_is_registered(m, dd->global_id, false)){
		m->opcode = OC_ACK_FAILURE;
		return;
	}
	m->global_id = dd->global_id;
	m->opcode = OC_ACK_SUCCESS;
	dd->global_id++;
}

/* Subscribes user to a new topic, creating it if it didn't exist.
 Sets the message opcode to the proper value depending on success. */
void subscribe_user(mom_message_t* m, dbms_data_t* dd) {
	if(!user_is_registered(m, dd->global_id, true)){
		m->opcode = OC_ACK_FAILURE;
		return;
	}
	char topic_path[PATH_MAX] = {0};
	sprintf(topic_path, "%s%s%s", TOPICS_DIR, m->topic, TOPIC_EXT);
	// Check user werent already subscribed to topic
	if(find_pair_id_topic(dd->inv_index, m->global_id, topic_path) > 0){
		printf("%d: Rejecting subscribing because %ld is already subscribed to %s\n", 
				getpid(), m->global_id, topic_path);
		m->opcode = OC_ACK_FAILURE;
		return;
	}
	// Write subscription at inverted index table
	// TODO: lock inv_index
	if(!write_pair_id_topic(dd->inv_index, m->global_id, topic_path)) {
		printf("%d: Error writing at invert index: %d\n", getpid(), errno);
		m->opcode = OC_ACK_FAILURE;
		return;
	}
	// If the topic and subscriber files don't exist, create them. 
	// Note the functions below silently do nothing if any exist.
	bool b = create_subscribers_file(topic_path);
	if((!b) || (!create_file(topic_path))){
		// Remove pair user topic from inverted index
		lseek(dd->inv_index, -SUBS_LINE_SIZE, SEEK_CUR);
		remove_pair_id_topic(dd->inv_index);
		m->opcode = OC_ACK_FAILURE;
		return;		
	}
	// Write user's global id at the topic's subscribers file
	char subs_path[PATH_MAX] = {0};
	topic_to_subscribers(topic_path, subs_path);
	if(!add_user_at_subscriber_file(subs_path, m->global_id, topic_path)) {
		// Remove pair user topic from inverted index
		lseek(dd->inv_index, -SUBS_LINE_SIZE, SEEK_CUR);
		remove_pair_id_topic(dd->inv_index);
		m->opcode = OC_ACK_FAILURE;
		return;
	}
	// If here, subscription was a success
	m->opcode = OC_ACK_SUCCESS;
}

/* Delivers aka forwards the message m to every subscriber of the topic
 with file located in topic_path. Returns false on any error. */
bool deliver_message_to_subscribers(mom_message_t* m, char* topic_path, dbms_data_t* dd){
	char subs_file[PATH_MAX] = {0};
	// Copy m into a forwarded message
	mom_message_t forwarded = {m->local_id, m->global_id, OC_DELIVERED," ", " ", m->mtype};
	strcpy(forwarded.topic, m->topic);
	strcpy(forwarded.payload, m->payload);
	// Retrieve subscribers file and open it
	topic_to_subscribers(topic_path, subs_file);
	// TODO: lock subs_file
	int fd = open(subs_file, O_RDONLY | O_CREAT, 0644);
	if(fd < 0) {
		printf("%d: Error opening subscribers file %s: %d\n", getpid(), topic_path, errno);
		return false;
	}
	// Iterate through file, finding users subscribed to topic_path
	long id; char t[TOPIC_LENGTH];
	while(read_pair_id_topic(fd, &id, t)) {
		bool have_to_forward = (strcmp(t, topic_path) == 0);
		have_to_forward &= (id != m->global_id); // Dont send messages to the sender
		if(have_to_forward) {
			// Really forward message
			forwarded.mtype = id;
			forwarded.local_id = id;
			msq_send(dd->msqid_s, &forwarded, sizeof(mom_message_t));
		}
	}
	
	close(fd);
	return true;
}

/* Pubblish the message payload into the proper topic. Creates
 the topic if it didn't exist. Sets the message opcode to the
 proper value depending on success. Also, delivers the message
 to every topic subscriber. */
void publish_message(mom_message_t* m, dbms_data_t* dd) {
	if(!user_is_registered(m, dd->global_id, true)){
		m->opcode = OC_ACK_FAILURE;
		return;
	}
	// If the topic and subscriber files don't exist, create them. 
	// Note the functions below silently do nothing if any exist.
	char topic_path[PATH_MAX];
	sprintf(topic_path, "%s%s%s", TOPICS_DIR, m->topic, TOPIC_EXT);
	bool b = create_subscribers_file(topic_path);
	if((!b) || (!create_file(topic_path))){
		m->opcode = OC_ACK_FAILURE;
		return;		
	}
	// Write m->payload to the topic file
	if(!publish_at_topic_file(topic_path, m->global_id, m->payload)){
		m->opcode = OC_ACK_FAILURE;
		return;		
	}
	// Deliver m to every user subscribed to the topic
	if(!deliver_message_to_subscribers(m, topic_path, dd)){
		m->opcode = OC_ACK_FAILURE;
		return;		
	}
	// If here, all went well
	m->opcode = OC_ACK_SUCCESS;
}

/* Erases a user id from the system. In other words, this function
 loops over the inverted index table and removes the user id from
 every topic related to them. Sets the opcode to the prover value. */
void unregister_user(mom_message_t* m, dbms_data_t* dd) {
	if(!user_is_registered(m, dd->global_id, true)){
		m->opcode = OC_ACK_FAILURE;
		return;
	}
	// Seek at the beginning of inv_table
	if(lseek(dd->inv_index, 0, SEEK_SET) < 0) {
		printf("%d: Error seeking in inverted index table: %d\n", getpid(), errno);
		m->opcode = OC_ACK_FAILURE;
		return;
	}	
	// Scan inverted index table
	long id; char t[TOPIC_LENGTH];
	while(read_pair_id_topic(dd->inv_index, &id, t)) {
		if(id == m->global_id) {
			// If here, user is subscribed at topic t
			// Hence, delete them from there
			char subs_file[PATH_MAX] = {0};
			topic_to_subscribers(t, subs_file);
			int fd = open(subs_file, O_RDWR | O_CREAT, 0644);
			if(fd < 0) {
				printf("%d: Error opening subscribers file %s: %d\n", getpid(), subs_file, errno);
				continue;
			}
			// First find offset at user position
			off_t p = find_pair_id_topic(fd, id, t);
			if(p < 0) { // Should not happen if file system is not corrupted
				printf("%d: Error finding %ld with topic %s in file %s: %d\n", getpid(), id, t, subs_file, errno);
				close(fd);
				continue;			
			}
			
			if(lseek(dd->inv_index, -SUBS_LINE_SIZE, SEEK_CUR) < 0) {
				printf("%d: Error seeking in inverted index table: %d\n", getpid(), errno);
				close(fd);
				continue;
			}
			if(lseek(fd, p, SEEK_SET) < 0) {
				printf("%d: Error seeking in subscribers file %s: %d\n", getpid(), subs_file, errno);
				close(fd);
				continue;
			}
			
			// Delete from subs_file and inv_index
			if((!remove_pair_id_topic(fd)) || (!remove_pair_id_topic(dd->inv_index))) {
				printf("%d: Error removing user %ld with topic %s: %d\n", getpid(), id, t, errno);
				close(fd);
				continue;
			}
			
			close(fd);
		}
	}
	
	// If here, no more deleting needed
	m->opcode = OC_ACK_SUCCESS;
}


void process_message(mom_message_t* m, dbms_data_t* dd) {
	// TODO: Fork here and change returns for exits
	switch(m->opcode) {
		// User has just created mom
		case OC_CREATE:
			register_user(m, dd);
			return;
		
		// User has subscribed
		case OC_SUBSCRIBE:
			m->mtype = m->global_id;
			subscribe_user(m, dd);
			return;
			
		// User has published smth
		case OC_PUBLISH:
			m->mtype = m->global_id;
			publish_message(m, dd);
			return;
		
		// User has destroyed mom
		case OC_DESTROY:
			m->mtype = m->global_id;
			unregister_user(m, dd);
			return;
		
		// Should never happen
		default:
			printf("%d: Warning! Received a not valid opcode of%d\n", getpid(), m->opcode);
			m->opcode = OC_ACK_FAILURE;
			return;
	}
	
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

// -------------------------------------------------------------------

/* @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ *
 * 
 *     BEHOLD THE AWESOMENESS AND BRIGHTNESS OF THE MAIN PROGRAM
 * 
 * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ */

// -------------------------------------------------------------------

int main(int argc, char* argv[]) {
	// Data struct for DBMS
	dbms_data_t dd = {0};
	dd.inv_index = create_inverted_index();
	if((dd.inv_index < 0) || (!create_directory(TOPICS_DIR)))
		return -1;
	
	// Use argv_parser to retrieve msqids
	ap_t* ap = ap_create_from_argv(argc, argv);
	if(!ap) {
		printf("%d: Error creating argv_parser:%d\n", getpid(), errno);
		exit(-1);
	}
	
	ap_get_int(ap, QUEUE_HANDLER, &dd.msqid_h);
	ap_get_int(ap, QUEUE_SENDER, &dd.msqid_s);
	
	ap_destroy(ap);
	
	dd.global_id = 1;
	
	set_handler();
	printf("Broker DBMS is up (PID: %d) !\n", getpid());
		
	// DBMS main loop
	while(keep_looping) {
		mom_message_t m = {0};
		msq_rcv(dd.msqid_h, &m, sizeof(mom_message_t), 0);
		if(!keep_looping)	break;
		printf("%d: DBMS now processing message...\n", getpid());
		// Message processing
		process_message(&m, &dd);
		// Forward response to sender
		msq_send(dd.msqid_s, &m, sizeof(mom_message_t));
	}
	
	printf("\nClosing DBMS...\n");
	close(dd.inv_index);
	return 0;
}

