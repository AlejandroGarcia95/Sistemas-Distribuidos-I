#define DOOR_AMOUNT 3
#define SIMULATE_FOREVER 0	// If setting this, increase delay timing
#define PEOPLE_AMOUNT 26
#define PEOPLE_TOUR 6
#define MUSEUM_CAPACITY 12
#define PERSON_PROB_SPAWNING 80
#define PERSON_PROB_TOUR 80
// Delay in microseconds
#define TIME_PERSON_SPAWN 1000
#define TIME_DOOR_RESP 1500
#define TIME_PERSON_INSIDE 10000
#define TIME_TOUR_TIMEOUT 60000
#define TIME_TOUR_DURATION 8000

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

