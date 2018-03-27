#define DOOR_AMOUNT 2
#define SIMULATE_FOREVER 0	// Set at your own risk
#define PEOPLE_AMOUNT 9
#define PEOPLE_TOUR 3
#define MUSEUM_CAPACITY 9
#define PERSON_PROB_SPAWNING 60
#define PERSON_PROB_TOUR 100

#define ACCEPTED 1
#define REJECTED 2
#define REQUEST_ENTER 3
#define REQUEST_EXIT 4
#define REQUEST_TOUR 5


struct message_ {
    long mtype;
    int msg_type; // ACCEPTED, REJECTED or REQUEST
};

typedef struct message_ message_t;

