#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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


// ---------------- Opcode treatment functions ----------------

/* Registers a new user, setting the mom_message sender_id field
 to the proper global_id of the sender. Sets the message opcode 
 to the proper value depending on success. */
void register_user(mom_message_t* m, long* global_id) {
	if(user_is_registered(m, global_id, false)){
		m->opcode = OC_ACK_FAILURE;
		return;
	}
	m->sender_id = *global_id;
	m->opcode = OC_ACK_SUCCESS;
	(*global_id)++;
}

/* Subscribes user to a new topic, creating it if it didn't exist.
 Sets the message opcode to the proper value depending on success. */
void subscribe_user(mom_message_t* m, int inv_index, long* global_id) {
	if(!user_is_registered(m, global_id, true)){
		m->opcode = OC_ACK_FAILURE;
		return;
	}
	char topic_path[PATH_MAX] = {0};
	sprintf(topic_path, "%s%s%s", TOPICS_DIR, m->topic, TOPIC_EXT);
	// Check user werent already subscribed to topic
	if(find_pair_id_topic(inv_index, m->sender_id, topic_path) > 0){
		printf("%d: Rejecting subscribing because %ld is already subscribed to %s\n", 
				getpid(), m->sender_id, topic_path);
		m->opcode = OC_ACK_FAILURE;
		return;
	}
	// Write subscription at inverted index table
	// TODO: lock inv_index
	if(!write_pair_id_topic(inv_index, m->sender_id, topic_path)) {
		printf("%d: Error writing at invert index: %d\n", getpid(), errno);
		m->opcode = OC_ACK_FAILURE;
		return;
	}
	// If the topic and subscriber files don't exist, create them. 
	// Note the functions below silently do nothing if any exist.
	bool b = create_subscribers_file(topic_path);
	if((!b) || (!create_file(topic_path))){
		// Remove pair user topic from inverted index
		lseek(inv_index, -SUBS_LINE_SIZE, SEEK_CUR);
		remove_pair_id_topic(inv_index);
		m->opcode = OC_ACK_FAILURE;
		return;		
	}
	// Write user's global id at the topic's subscribers file
	char subs_path[PATH_MAX] = {0};
	topic_to_subscribers(topic_path, subs_path);
	if(!add_user_at_subscriber_file(subs_path, m->sender_id, topic_path)) {
		// Remove pair user topic from inverted index
		lseek(inv_index, -SUBS_LINE_SIZE, SEEK_CUR);
		remove_pair_id_topic(inv_index);
		m->opcode = OC_ACK_FAILURE;
		return;
	}
	// If here, subscription was a success
	m->opcode = OC_ACK_SUCCESS;
}

/* Delivers aka forwards the message m to every subscriber of the topic
 with file located in topic_path. Returns false on any error. */
bool deliver_message_to_subscribers(mom_message_t* m, char* topic_path){
	char subs_file[PATH_MAX] = {0};
	// Copy m into a forwarded message
	mom_message_t forwarded = {m->sender_id, OC_DELIVERED," ", " ", m->mtype};
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
		have_to_forward &= (id != m->sender_id); // Dont send messages to the sender
		if(have_to_forward) {
			// TODO: Really forward message
			printf("I have to forward message to %ld (payload: %s)\n", id, forwarded.payload);
		}
	}
	
	close(fd);
	return true;
}

/* Pubblish the message payload into the proper topic. Creates
 the topic if it didn't exist. Sets the message opcode to the
 proper value depending on success. Also, delivers the message
 to every topic subscriber. */
void publish_message(mom_message_t* m, long* global_id) {
	if(!user_is_registered(m, global_id, true)){
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
	if(!publish_at_topic_file(topic_path, m->sender_id, m->payload)){
		m->opcode = OC_ACK_FAILURE;
		return;		
	}
	// Deliver m to every user subscribed to the topic
	if(!deliver_message_to_subscribers(m, topic_path)){
		m->opcode = OC_ACK_FAILURE;
		return;		
	}
	// If here, all went well
	m->opcode = OC_ACK_SUCCESS;
}

/* Erases a user id from the system. In other words, this function
 loops over the inverted index table and removes the user id from
 every topic related to them. Sets the opcode to the prover value. */
void unregister_user(mom_message_t* m, int inv_index, long* global_id) {
	if(!user_is_registered(m, global_id, true)){
		m->opcode = OC_ACK_FAILURE;
		return;
	}
	// Seek at the beginning of inv_table
	if(lseek(inv_index, 0, SEEK_SET) < 0) {
		printf("%d: Error seeking in inverted index table: %d\n", getpid(), errno);
		m->opcode = OC_ACK_FAILURE;
		return;
	}	
	// Scan inverted index table
	long id; char t[TOPIC_LENGTH];
	while(read_pair_id_topic(inv_index, &id, t)) {
		if(id == m->sender_id) {
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
			
			if(lseek(inv_index, -SUBS_LINE_SIZE, SEEK_CUR) < 0) {
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
			if((!remove_pair_id_topic(fd)) || (!remove_pair_id_topic(inv_index))) {
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

void test_stuff(int inv_index, long* global_id);

// -------------------------------------------------------------------

/* @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ *
 * 
 *     BEHOLD THE AWESOMENESS AND BRIGHTNESS OF THE MAIN PROGRAM
 * 
 * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ * @ */

// -------------------------------------------------------------------

int main(int argc, char* argv[]) {
	// TODO: Use argv_parser to receive msqids
	int inv_index = create_inverted_index();
	if((inv_index < 0) || (!create_directory(TOPICS_DIR)))
		return -1;
	long global_id = 1;
	
	// TODO: Remove test and add loop for reading/writing to msqs
	
	test_stuff(inv_index, &global_id);
	
		
	close(inv_index);
	return 0;
}


void test_stuff(int inv_index, long* global_id){
	// m = {sender_id, opcode, topic, payload, mtype}
	mom_message_t m1 = {10, OC_CREATE, "SensorA/Measures/Temperature", "29 Celsius", 22};
	mom_message_t m2 = {11, OC_CREATE, "SensorA/Measures/Temperature", "23 Celsius", 33};
	mom_message_t m3 = {12, OC_CREATE, "SensorA/Measures/Temperature", "-10 Celsius", 44};
	mom_message_t m4 = {13, OC_CREATE, "SensorA/Measures/Temperature", "1 atm", 55};
	mom_message_t m5 = {14, OC_CREATE, "SensorA/Measures/Temperature", "88%", 66};
	mom_message_t m6 = {15, OC_CREATE, "SensorA/Measures/Temperature", "31 Celsius", 77};
	// Create six users
	register_user(&m1, global_id);
	register_user(&m2, global_id);
	register_user(&m3, global_id);
	register_user(&m4, global_id);
	register_user(&m5, global_id);
	register_user(&m6, global_id);
	// Subscribe all six users to topics:
	// Every user will subscribe to SensorA/Measures/Temperature
	// Only users 1, 5, 6 will subscribe to SensorB/Measures/Temperature
	// Only users 2, 4 will subscribe to SensorB/Measures/Pressure
	// Only users 2, 3 will subscribe to SensorB/Measures/Humidity
	subscribe_user(&m1, inv_index, global_id);
	subscribe_user(&m2, inv_index, global_id);
	subscribe_user(&m3, inv_index, global_id);
	subscribe_user(&m4, inv_index, global_id);
	subscribe_user(&m5, inv_index, global_id);
	subscribe_user(&m6, inv_index, global_id);
	sprintf(m1.topic, "SensorB/Measures/Temperature");
	sprintf(m5.topic, "SensorB/Measures/Temperature");
	sprintf(m6.topic, "SensorB/Measures/Temperature");
	subscribe_user(&m1, inv_index, global_id);
	subscribe_user(&m5, inv_index, global_id);
	subscribe_user(&m6, inv_index, global_id);
	sprintf(m2.topic, "SensorB/Measures/Pressure");
	sprintf(m4.topic, "SensorB/Measures/Pressure");
	subscribe_user(&m2, inv_index, global_id);
	subscribe_user(&m4, inv_index, global_id);
	sprintf(m2.topic, "SensorB/Measures/Humidity");
	sprintf(m3.topic, "SensorB/Measures/Humidity");
	subscribe_user(&m2, inv_index, global_id);
	subscribe_user(&m3, inv_index, global_id);
	// Publish some stuff:
	// 1 will publish to SensorB/Measures/Temperature => 5, 6 will receive
	// 5 will publish to SensorB/Measures/Humidity => 2, 3 will receive
	// 4 will publish to SensorB/Measures/Pressure => 2 will receive
	// 3 will publish to SensorA/Measures/Temperature => Everyone except 3 will receive
	// 2 will publish to SensorB/Measures/Temperature => 1, 5, 6 will receive
	publish_message(&m1, global_id);
	sprintf(m5.topic, "SensorB/Measures/Humidity");
	publish_message(&m5, global_id);
	sprintf(m4.topic, "SensorB/Measures/Pressure");
	publish_message(&m4, global_id);
	sprintf(m3.topic, "SensorA/Measures/Temperature");
	publish_message(&m3, global_id);
	sprintf(m2.topic, "SensorB/Measures/Temperature");
	publish_message(&m2, global_id);
	// Remove users 2 and 5 :o
	unregister_user(&m2, inv_index, global_id);
	unregister_user(&m5, inv_index, global_id);
	
}
