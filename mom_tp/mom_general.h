/* 
 * 
 * FILE FOR DEFINING GENERAL STUFF FOR ALL MOM
 * 
 * */

#define QUEUE_REQUESTER "queue_req.temp"
#define QUEUE_RESPONSER "queue_resp.temp"


#define PAYLOAD_SIZE 255
#define TOPIC_LENGTH 140

typedef enum opcode_ {
	OC_CREATE, 
	OC_DESTROY, 
	OC_PUBLISH, 
	OC_SUBSCRIBE,
	OC_ACK_SUCCESS, 
	OC_ACK_FAILURE,
	OC_DELIVERED // Used by broker when sending message to all subscribers on topic
	} opcode_t;
	


typedef  struct mom_message_ {
	long sender_id;
	opcode_t opcode;
	char topic[TOPIC_LENGTH];
	
	char payload[PAYLOAD_SIZE];
	
	long mtype;
	
} mom_message_t;



// Debug purposes only
void print_message(mom_message_t m){
	char* oc_str[] = 	{"CREATE", "DESTROY", "PUBLISH", "SUBSCRIBE", 
						"ACK_SUCCESS", "ACK_FAILURE", "DELIVERED"};
	printf("---------------------------------------------------\n");
	printf("SENDER ID: %ld\n", m.sender_id);
	printf("OPCODE: %s\n", oc_str[(int) m.opcode]);
	printf("TOPIC: %s\n", m.topic);
	printf("PAYLOAD: %s \n", m.payload);
	printf("\nMTYPE: %ld\n", m.mtype);
	printf("---------------------------------------------------\n");
}
