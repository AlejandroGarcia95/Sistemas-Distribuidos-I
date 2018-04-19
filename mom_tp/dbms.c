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
 if such file already exists. Returns false on failure. */
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

/* Creates an empty .subs file in the given path. Silently does
 nothing if such file already exists. If this function receives 
 path with a file ending (i.e. home/user/docs/file.txt) then the
 .subs file is created on the deepest directory level possible
 (i.e. home/user/docs/). Return false on failure, true otherwise.*/
bool create_subscribers_file(char* path){
	char aux[PATH_MAX], file_path[PATH_MAX];

	strcpy(aux, path);
	// Turn last slash NULL
	for(int i = strlen(aux) - 1; i >= 0; i--)
		if(aux[i] == '/') {
			aux[i + 1] = '\0';
			break;
		}
	// Create .subs if it doesnt exist
	sprintf(file_path, "%s%s", aux, SUBS_FILE);
	return create_file(file_path);
}

/* Creates the inverted_index file, used to store the topics each
 * user has subscribed to. Returns the index file descriptor. */
int create_inverted_index() {
	int fd = open(INVERTED_INDEX, O_RDWR | O_APPEND | O_CREAT, 0644);
	if(fd < 0) 
		printf("%d: Error creating %s: %d\n", getpid(), INVERTED_INDEX, errno);
	
	return fd;
}

/* Adds a user whose global id is user_id to the subscribers file
 located at file_path. Returns true on success, false otherwise. */
bool add_user_subscriber_file(char* file_path, long user_id, char* topic) {
	// TODO: lock file_path
	int fd = open(file_path, O_RDWR | O_APPEND | O_CREAT, 0644);
	if(fd < 0) {
		printf("%d: Error opening subscribers file %s: %d\n", getpid(), file_path, errno);
		return false;
	}
	char aux[26] = {0};
	sprintf(aux, "%ld %s\n", user_id, topic);
	if(write(fd, aux, strlen(aux)) < 0) {
		printf("%d: Error writing to subscribers file %s: %d\n", getpid(), file_path, errno);
		return false;
	}
	return true;
}

// ---------------- Message treatment functions ----------------

/* Checks if user has ever registered before. Returns true if the
 * user's really registered, false if not. */
bool user_is_registered(mom_message_t* m, int* global_id, bool print_warning) { 
	if(m->sender_id < (*global_id))
		return true;
	if(print_warning)
		printf("%d: Warning! Some not registered user was trying to make a query!\n", getpid());
	return false;
}

/* Registers a new user, setting the mom_message sender_id field
 * to the proper global_id of the sender.*/
void register_user(mom_message_t* m, int* global_id) {
	if(user_is_registered(m, global_id, false)){
		m->opcode = OC_ACK_FAILURE;
		return;
	}
	m->sender_id = *global_id;
	m->opcode = OC_ACK_SUCCESS;
	(*global_id)++;
}

void subscribe_user(mom_message_t* m, int inv_index, int* global_id) {
	if(!user_is_registered(m, global_id, true)){
		m->opcode = OC_ACK_FAILURE;
		return;
	}
	// TODO: Check if user was subscribed to topic
	// Write subscription at inverted index table
	// TODO: lock inv_index
	char aux[TOPIC_LENGTH + 26] = {0};
	sprintf(aux, "%ld %s%s%s\n", m->sender_id, TOPICS_DIR, m->topic, TOPIC_EXT);
	if(write(inv_index, aux, strlen(aux)) < 0) {
		printf("%d: Error writing at invert index: %d\n", getpid(), errno);
		m->opcode = OC_ACK_FAILURE;
		return;
	}
	// If the topic and subscriber files don't exist, create them. 
	// Note the functions below silently do nothing if any exist.
	sprintf(aux, "%s%s%s", TOPICS_DIR, m->topic, TOPIC_EXT);
	bool b = create_subscribers_file(aux);
	if((!b) || (!create_file(aux))){
		// TODO: Remove pair user topic from inverted index
		m->opcode = OC_ACK_FAILURE;
		return;		
	}
	// Write user's global id at the topic's subscribers file
	
	// Turn last slash NULL
	int last_slash;
	for(last_slash = strlen(aux) - 1; last_slash >= 0; last_slash--)
		if(aux[last_slash] == '/') {
			aux[last_slash] = '\0';
			break;
		}
	char subs_path[PATH_MAX] = {0};
	sprintf(subs_path, "%s/%s", aux, SUBS_FILE);
	if(!add_user_subscriber_file(subs_path, m->sender_id, &aux[last_slash + 1])) {
		// TODO: Remove pair user topic from inverted index
		m->opcode = OC_ACK_FAILURE;
		return;
	}
	// If here, subscription was a success
	m->opcode = OC_ACK_SUCCESS;
}

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
	int global_id = 1;
	
	// TODO: Change for loop and read/write to msqs
	
	// m = {sender_id, opcode, topic, payload, mtype}
	mom_message_t m = {10, OC_CREATE, "SensorA/Measures/Temperature", "29 Celsius", 15};
	register_user(&m, &global_id);
	m.opcode = OC_SUBSCRIBE;
	subscribe_user(&m, inv_index, &global_id);
	m.sender_id = 11;
	register_user(&m, &global_id);
	subscribe_user(&m, inv_index, &global_id);
	sprintf(m.topic, "SensorA/Measures/Pressure");
	subscribe_user(&m, inv_index, &global_id);
	sprintf(m.topic, "SensorB/Measures/Pressure");
	subscribe_user(&m, inv_index, &global_id);
	
	close(inv_index);
	return 0;
}
