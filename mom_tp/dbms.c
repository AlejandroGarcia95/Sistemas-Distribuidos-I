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
#include "libs/msq.h"
#include "libs/argv_parser.h"

/* Reality is merely an illusion, albeit a very persistent one.*/

#define TOPICS_DIR "Topics/"
#define INVERTED_INDEX "inv_index.txt"
#define TOPIC_EXT ".topic"
#define SUBS_FILE "subscribers.subs"

#define ID_MAX_LENGTH 10
#define SUBS_LINE_SIZE (ID_MAX_LENGTH + TOPIC_LENGTH + 2)

// ------------ Auxiliar functions so seriously needed ------------

/* Creates a directory in a beautifully iterative way. Silently 
 does nothing if a subdirectory already exists. If this function
 receives path with a file ending (i.e. home/user/docs/file.txt)
 it DOES NOT create it (only creates the directory to contain it!). */
bool create_directory(char *path) {
	char aux[PATH_MAX];

	strcpy(aux, path);
	// Turn last slash NULL
	for(int i = strlen(aux) - 1; i >= 0; i--)
		if(aux[i] == '/') {
			aux[i + 1] = '\0';
			break;
		}
	// Iterate over directories
	for(int i = 0; i < strlen(aux); i++)
		if(aux[i] == '/') {
			aux[i] = '\0';
			int r = mkdir(aux, S_IRWXU);
			if((r < 0) && (errno != EEXIST)) {
				printf("%d: Error creating directory %s: %d\n", getpid(), aux, errno);
				return false;
			}
			aux[i] = '/';
		}
	return true;
}

/* Creates a file on the given file_path. Silently does nothing
 if such file already exists. Returns true on success. (Note: if
 the file already exists, does nothing and return true). */
bool create_file(char* file_path) {
	bool b = create_directory(file_path);
	if(!b)	return false;
	int r = mknod(file_path, S_IFREG|0777, 0);
	if((r < 0) && (errno != EEXIST)){
		printf("%d: Error creating subscriber file %s: %d\n", getpid(), file_path, errno);
		return false;
	}
	return true;
}

/* Given a topic file of path topic_path, this functions writes
 to subs_path the path of the proper subscribers path. */
void topic_to_subscribers(char* topic_path, char* subs_path){
	char aux[PATH_MAX] = {0};
	strcpy(aux, topic_path);
	// Turn last slash NULL
	int last_slash;
	for(last_slash = strlen(aux) - 1; last_slash >= 0; last_slash--)
		if(aux[last_slash] == '/') {
			aux[last_slash] = '\0';
			break;
		}
	
	sprintf(subs_path, "%s/%s", aux, SUBS_FILE);
}

/* Creates a .subs file related to the topic file given.
 If the .subs file already exists, silently does nothing
 and return true. False is returned on error. */
bool create_subscribers_file(char* topic_path){
	char subs_path[PATH_MAX];

	topic_to_subscribers(topic_path, subs_path);
	return create_file(subs_path);
}


/* Creates the inverted_index file, used to store the topics each
 user has subscribed to. Returns the index file descriptor. */
int create_inverted_index() {
	int fd = open(INVERTED_INDEX, O_RDWR | O_APPEND | O_CREAT, 0644);
	if(fd < 0) 
		printf("%d: Error creating %s: %d\n", getpid(), INVERTED_INDEX, errno);
	
	return fd;
}

/* Writes to PREVIOUSLY OPENED FILE fd a pair user_id vs topic_path, 
 meaning a subscription. Returns true on success, false otherwise.*/
bool write_pair_id_topic(int fd, long user_id, char* topic_path) {
	char aux[SUBS_LINE_SIZE] = {0};
	sprintf(aux, "%ld %s", user_id, topic_path);
	int len = strlen(aux);
	// Padding with spaces for fixed size!
	for(int i = len; i < (SUBS_LINE_SIZE - 1); i++)
		aux[i] = ' ';
	aux[SUBS_LINE_SIZE - 1] = '\n';
	return (write(fd, aux, SUBS_LINE_SIZE) > 0);
}

/* Reads from PREVIOUSLY OPENED FILE fd a pair user_id vs topic_path, 
 meaning a subscription. Returns true on success, false otherwise.*/
bool read_pair_id_topic(int fd, long* user_id, char* topic_path) {
	char aux[SUBS_LINE_SIZE] = {0};
	if(read(fd, aux, SUBS_LINE_SIZE) <= 0)
		return false;
	// Parse aux with sscanf magic
	sscanf(aux, "%ld %s ", user_id, topic_path);
	return true;
}


/* Adds a user whose global id is user_id to the subscribers file
 located at file_path. Returns true on success, false otherwise. */
bool add_user_at_subscriber_file(char* file_path, long user_id, char* topic_path) {
	// TODO: lock file_path
	int fd = open(file_path, O_RDWR | O_APPEND | O_CREAT, 0644);
	if(fd < 0) {
		printf("%d: Error opening subscribers file %s: %d\n", getpid(), file_path, errno);
		return false;
	}
	
	if(!write_pair_id_topic(fd, user_id, topic_path)) {
		printf("%d: Error writing to subscribers file %s: %d\n", getpid(), file_path, errno);
		return false;
	}
	close(fd);
	return true;
}

/* Adds a message published by user with global id given to the topic
 located at topic_path. Returns true on success, false otherwise. */
bool publish_at_topic_file(char* topic_path, long user_id, char* message) {
	// TODO: lock topic_path
	int fd = open(topic_path, O_RDWR | O_APPEND | O_CREAT, 0644);
	if(fd < 0) {
		printf("%d: Error opening topics file %s: %d\n", getpid(), topic_path, errno);
		return false;
	}
	char aux[PAYLOAD_SIZE + ID_MAX_LENGTH] = {0};
	sprintf(aux, "%ld %s\n", user_id, message);
	if(write(fd, aux, strlen(aux)) < 0) {
		printf("%d: Error writing at topics file %s: %d\n", getpid(), topic_path, errno);
		return false;
	}
	close(fd);
	return true;
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
		have_to_forward &= (id != m->sender_id); // To not send messages to the sender
		if(have_to_forward) {
			// TODO: Really forward message
			printf("I have to forward message to %ld (payload: %s)\n", id, forwarded.payload);
		}
	}
	
	close(fd);
	return true;
}

/* Checks if user has ever registered before. Returns true if the
 user's really registered, false if not. */
bool user_is_registered(mom_message_t* m, long* global_id, bool print_warning) { 
	if(m->sender_id < (*global_id))
		return true;
	if(print_warning)
		printf("%d: Warning! Some not registered user was trying to make a query!\n", getpid());
	return false;
}

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
	// TODO: Check if user was subscribed to topic
	// Write subscription at inverted index table
	// TODO: lock inv_index
	char topic_path[PATH_MAX] = {0};
	sprintf(topic_path, "%s%s%s", TOPICS_DIR, m->topic, TOPIC_EXT);
	if(!write_pair_id_topic(inv_index, m->sender_id, topic_path)) {
		printf("%d: Error writing at invert index: %d\n", getpid(), errno);
		m->opcode = OC_ACK_FAILURE;
		return;
	}
	// If the topic and subscriber files don't exist, create them. 
	// Note the functions below silently do nothing if any exist.
	bool b = create_subscribers_file(topic_path);
	if((!b) || (!create_file(topic_path))){
		// TODO: Remove pair user topic from inverted index
		m->opcode = OC_ACK_FAILURE;
		return;		
	}
	// Write user's global id at the topic's subscribers file
	char subs_path[PATH_MAX] = {0};
	topic_to_subscribers(topic_path, subs_path);
	if(!add_user_at_subscriber_file(subs_path, m->sender_id, topic_path)) {
		// TODO: Remove pair user topic from inverted index
		m->opcode = OC_ACK_FAILURE;
		return;
	}
	// If here, subscription was a success
	m->opcode = OC_ACK_SUCCESS;
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
	deliver_message_to_subscribers(m, topic_path);
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
	
}
