#define DOOR_AMOUNT 6
#define SIMULATE_FOREVER 0	// Set at your own risk
#define PEOPLE_AMOUNT 60
#define MUSEUM_CAPACITY 9
#define PERSON_PROB_SPAWNING 60


#define ACCEPTED 1
#define REJECTED 2
#define REQUEST_ENTER 3
#define REQUEST_EXIT 4


struct message_ {
    long mtype;
    int msg_type; // ACCEPTED, REJECTED or REQUEST
};

typedef struct message_ message_t;

