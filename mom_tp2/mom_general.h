/* 
 * 
 * FILE FOR DEFINING GENERAL STUFF FOR ALL MOM
 * 
 * */

#define QUEUE_REQUESTER "queue_req.temp"
#define QUEUE_RESPONSER "queue_resp.temp"
#define QUEUE_FORWARDER "queue_forw.temp"


#define QUEUE_HANDLER "queue_handler.temp"
#define QUEUE_SENDER "queue_sender.temp"
#define QUEUE_RM "queue_rm.temp"

#define NEXT_IP "next_ip"
#define NEXT_PORT "next_port"
#define BROKER_ID "self_id"
#define BROKER_AMOUNT "amount"

#define SOCKET_FD "socket"

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
	OC_SEPPUKU = 8, // Used by mom_daemon to tell handler to gracefully die
	OC_UNSUBSCRIBE = 9,
	OC_RECEIVE = 10, // To be send to forwarder
	OC_BR_CONNECT = 11, // Used by ring_master for starting connection
	OC_BR_PUBLISH = 12 // Used by ring_master for publishing into other broker
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
	char* oc_str[] = 	{"", "CREATE", "DESTROY", "PUBLISH", "SUBSCRIBE", "ACK_SUCCESS", 
						"ACK_FAILURE", "DELIVERED", "SEPPUKU", "UNSUBSCRIBE", "RECEIVE",
						"BR_CONNECT", "BR_PUBLISH"};
	printf("---------------------------------------------------\n");
	printf("LOCAL ID: %ld\n", m.local_id);
	printf("GLOBAL ID: %ld\n", m.global_id);
	printf("OPCODE: %s\n", oc_str[(int) m.opcode]);
	printf("TOPIC: %s\n", m.topic);
	printf("PAYLOAD: %s \n", m.payload);
	printf("\nMTYPE: %ld\n", m.mtype);
	printf("---------------------------------------------------\n");
}
