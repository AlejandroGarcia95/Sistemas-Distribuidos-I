CFLAGS := -g -std=c99
VFLAGS := --leak-check=full --show-leak-kinds=all --track-origins=yes
LIBS_PATH = libs
LIBS_C = $(wildcard $(LIBS_PATH)/*.c)
LIBS_O = $(patsubst $(LIBS_PATH)/%.c, $(LIBS_PATH)/%.o, $(LIBS_C))
MOM_API = mom.o
PROGRAM = user_main
DAEMON_PROCESSES = mom_responser mom_requester mom_forwarder
MOM_DAEMON = mom_daemon
BROKER_PROCESSES = dbms broker_handler broker_sender
BROKER = broker_server


# ----------------------------------------------------------------------

all: clean $(PROGRAM) $(MOM_DAEMON) $(BROKER)


$(PROGRAM): $(LIBS_O) $(MOM_API) $(PROGRAM).c
	gcc $(CFLAGS) -o $(PROGRAM) $^


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

topiclean:
	rm -r -f Topics
	rm -f inv_index.txt
	rm -f CoordTopic.txt
	rm -f *.dat

run_broker: $(BROKER)
	./$(BROKER) 127.0.0.1 8080

run_daemon: $(MOM_DAEMON)
	./$(MOM_DAEMON) 127.0.0.1 8080

clean:
	rm -f $(PROGRAM) *.o *.temp $(MOM_DAEMON) $(DAEMON_PROCESSES) $(BROKER) $(BROKER_PROCESSES)
	rm -f $(LIBS_O)
	rm -r -f Topics
	rm -f inv_index.txt
	sh ipcrma.sh

valgrind: clean $(PROGRAM)
	valgrind $(VFLAGS) ./$(PROGRAM)

.PHONY: clean


