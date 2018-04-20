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

// --------- Auxiliar functions to deal with DB File System ---------


#define TOPICS_DIR "Topics/"
#define INVERTED_INDEX "inv_index.txt"
#define TOPIC_EXT ".topic"
#define SUBS_FILE "subscribers.subs"

#define ID_MAX_LENGTH 10
#define SUBS_LINE_SIZE (ID_MAX_LENGTH + TOPIC_LENGTH + 2)


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
	int fd = open(INVERTED_INDEX, O_RDWR | O_CREAT, 0644);
	if(fd < 0) 
		printf("%d: Error creating %s: %d\n", getpid(), INVERTED_INDEX, errno);
	if(lseek(fd, 0, SEEK_END) < 0) {
		printf("%d: Error seeking end of index table: %d\n", getpid(), errno);
		return -1;
	}
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

/* Finds from PREVIOUSLY OPENED FILE fd a pair user_id vs topic_path. 
 Returns the off_t from SEEK_SET where it was found, or a negative
 number in case of error or not finding the pair. */
off_t find_pair_id_topic(int fd, long user_id, char* topic_path) {
	if(lseek(fd, 0, SEEK_SET) < 0) {
		printf("%d: Error seeking at subscribers file: %d\n", getpid(), errno);
		return -1;
	}
	// Iterate through file until finding the desired line
	long id; char t[TOPIC_LENGTH];
	while(read_pair_id_topic(fd, &id, t)) {
		bool is_desired = (strcmp(t, topic_path) == 0);
		is_desired &= (id == user_id);
		if(is_desired) {
			off_t loc = lseek(fd, -SUBS_LINE_SIZE, SEEK_CUR);
			if(loc < 0) {
				printf("%d: Error seeking at subscribers file: %d\n", getpid(), errno);
				return -1;
			}
			return loc;
		}
	}
	// If here, the pair isnt in the file
	return -1;
	
}

/* Deletes from PREVIOUSLY OPENED FILE fd the NEXT pair user_id vs 
 topic_path in the file. Returns true on success, false otherwise.*/
bool remove_pair_id_topic(int fd) {
	off_t next_pair = lseek(fd, 0, SEEK_CUR);
	// To remove pair, first we copy last pair into next pair
	off_t l = lseek(fd, -SUBS_LINE_SIZE, SEEK_END);
	if((next_pair < 0) || (l < 0)) {
		printf("%d: Error seeking in subscribers file: %d\n", getpid(), errno);
		return false;
	}
	char last_pair[SUBS_LINE_SIZE] = {0};
	if(read(fd, last_pair, SUBS_LINE_SIZE) < 0) {
		printf("%d: Error reading from subscribers file: %d\n", getpid(), errno);
		return false;
	}
	if(lseek(fd, next_pair, SEEK_SET) < 0) {
		printf("%d: Error seeking in subscriber file: %d\n", getpid(), errno);
		return false;
	}
	if(write(fd, last_pair, SUBS_LINE_SIZE) < 0) {
		printf("%d: Error writing to subscribers file: %d\n", getpid(), errno);
		return false;
	}
	if(lseek(fd, -SUBS_LINE_SIZE, SEEK_CUR) < 0) {
		printf("%d: Error seeking in subscriber file: %d\n", getpid(), errno);
		return false;
	}
	// Now just truncate file to size l
	if(ftruncate(fd, l) < 0) {
		printf("%d: Error truncating subscribers file: %d\n", getpid(), errno);
		return false;
	}
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


/* Checks if user has ever registered before. Returns true if the
 user's really registered, false if not. */
bool user_is_registered(mom_message_t* m, long* global_id, bool print_warning) { 
	if(m->sender_id < (*global_id))
		return true;
	if(print_warning)
		printf("%d: Warning! Some not registered user was trying to make a query!\n", getpid());
	return false;
}

