CFLAGS := -g -std=c99
VFLAGS := --leak-check=full --show-leak-kinds=all --track-origins=yes
LIBS_PATH = libs
LIBS_C = $(wildcard $(LIBS_PATH)/*.c)
LIBS_O = $(patsubst $(LIBS_PATH)/%.c, $(LIBS_PATH)/%.o, $(LIBS_C))
MOM_API = mom.o
PROGRAM = user_main
DAEMON_PROCESSES = mom_responser mom_requester
MOM_DAEMON = mom_daemon
BROKER_PROCESSES = dbms broker_handler broker_sender
BROKER = broker_server
MUSEUM_API = museum_general.o
MUSEUM = museum_main
MUSEUM_PROCESSES = museum_door museum_person museum_guide museum_tour_timeout

# ----------------------------------------------------------------------

all: clean $(PROGRAM) $(MOM_DAEMON) $(BROKER) $(MUSEUM)


$(PROGRAM): $(LIBS_O) $(MOM_API) $(PROGRAM).c
	gcc $(CFLAGS) -o $(PROGRAM) $^


$(MUSEUM): $(LIBS_O) $(MUSEUM).c $(MUSEUM_PROCESSES:=.c) $(MOM_API) $(MUSEUM_API)
	for i in $(MUSEUM_PROCESSES) ; do gcc $(CFLAGS) -o $$i $$i.c $(LIBS_O) $(MOM_API) $(MUSEUM_API); done
	gcc $(CFLAGS) -o $(MUSEUM) $(LIBS_O) $(MOM_API) $(MUSEUM_API) $(MUSEUM).c

$(BROKER): $(LIBS_O) $(BROKER).c $(BROKER_PROCESSES:=.c)
	for i in $(BROKER_PROCESSES) ; do gcc $(CFLAGS) -o $$i $$i.c $(LIBS_O) ; done
	gcc $(CFLAGS) -o $(BROKER) $(LIBS_O) $(BROKER).c

$(MOM_DAEMON): $(LIBS_O) $(MOM_DAEMON).c $(DAEMON_PROCESSES:=.c)
	for i in $(DAEMON_PROCESSES) ; do gcc $(CFLAGS) -o $$i $$i.c $(LIBS_O) ; done
	gcc $(CFLAGS) -o $(MOM_DAEMON) $(LIBS_O) $(MOM_DAEMON).c

	
run: clean $(PROGRAM) $(DAEMON_PROCESSES) $(MOM_DAEMON)
	./$(PROGRAM)

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@ 

clean:
	rm -f $(PROGRAM) *.o *.temp $(MOM_DAEMON) $(DAEMON_PROCESSES) $(BROKER) $(BROKER_PROCESSES) $(MUSEUM) $(MUSEUM_PROCESSES)
	rm -f $(LIBS_O)
	rm -f *~ *.pyc
	sh ipcrma.sh

valgrind: clean $(PROGRAM)
	valgrind $(VFLAGS) ./$(PROGRAM)

.PHONY: clean


