/* 
 * 
 * FILE FOR DEFINING GENERAL STUFF FOR ALL MOM
 * 
 * */

#define QUEUE_REQUESTER "queue_req.temp"
#define QUEUE_RESPONSER "queue_resp.temp"


#define QUEUE_HANDLER "queue_handler.temp"
#define QUEUE_SENDER "queue_sender.temp"

#define SOCKET_FD "socket"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT "8080"

#define PAYLOAD_SIZE 255
#define TOPIC_LENGTH 100

typedef enum opcode_ {
	OC_CREATE = 1, 
	OC_DESTROY = 2, 
	OC_PUBLISH = 3, 
	OC_SUBSCRIBE = 4,
	OC_ACK_SUCCESS = 5, 
	OC_ACK_FAILURE = 6,
	OC_DELIVERED = 7, // Used by broker when sending message to all subscribers on topic
	OC_SEPPUKU = 8 // Used by mom_daemon to tell handler to gracefully die
	} opcode_t;
	


typedef  struct mom_message_ {
	long mtype;
	
	long local_id;
	long global_id;
	opcode_t opcode;
	char topic[TOPIC_LENGTH];
	
	char payload[PAYLOAD_SIZE];
	
} mom_message_t;



// Debug purposes only
void print_message(mom_message_t m){
	char* oc_str[] = 	{"", "CREATE", "DESTROY", "PUBLISH", "SUBSCRIBE", 
						"ACK_SUCCESS", "ACK_FAILURE", "DELIVERED", "SEPPUKU"};
	printf("---------------------------------------------------\n");
	printf("LOCAL ID: %ld\n", m.local_id);
	printf("GLOBAL ID: %ld\n", m.global_id);
	printf("OPCODE: %s\n", oc_str[(int) m.opcode]);
	printf("TOPIC: %s\n", m.topic);
	printf("PAYLOAD: %s \n", m.payload);
	printf("\nMTYPE: %ld\n", m.mtype);
	printf("---------------------------------------------------\n");
}
