CFLAGS := -g -std=c99
VFLAGS := --leak-check=full --show-leak-kinds=all --track-origins=yes
LIBS_PATH = libs
LIBS_C = $(wildcard $(LIBS_PATH)/*.c)
LIBS_O = $(patsubst $(LIBS_PATH)/%.c, $(LIBS_PATH)/%.o, $(LIBS_C))
SRCS = mom.o
PROGRAM = user_main
DAEMON_PROCESSES = mom_responser mom_requester
MOM_DAEMON = mom_daemon
BROKER_PROCESSES = dbms broker_handler broker_sender
BROKER = broker_server


# ----------------------------------------------------------------------

all: clean $(PROGRAM) $(DAEMON_PROCESSES) $(MOM_DAEMON) $(BROKER) $(BROKER_PROCESSES)


$(PROGRAM): $(LIBS_O) $(SRCS) $(PROGRAM).c
	gcc $(CFLAGS) -o $(PROGRAM) $^

$(BROKER): $(LIBS_O) $(BROKER).c
	gcc $(CFLAGS) -o $(BROKER) $^

$(MOM_DAEMON): $(LIBS_O) $(MOM_DAEMON).c
	gcc $(CFLAGS) -o $(MOM_DAEMON) $^

mom_requester: $(LIBS_O) mom_requester.c
	gcc $(CFLAGS) -o mom_requester $^

dbms: $(LIBS_O) dbms.c
	gcc $(CFLAGS) -o dbms $^

broker_handler: $(LIBS_O) broker_handler.c
	gcc $(CFLAGS) -o broker_handler $^

broker_sender: $(LIBS_O) broker_sender.c
	gcc $(CFLAGS) -o broker_sender $^

mom_responser: $(LIBS_O) mom_responser.c
	gcc $(CFLAGS) -o mom_responser $^
	
run: clean $(PROGRAM) $(DAEMON_PROCESSES) $(MOM_DAEMON)
	./$(PROGRAM)

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@ 

clean:
	rm -f $(PROGRAM) *.o *.temp $(MOM_DAEMON) $(DAEMON_PROCESSES) $(BROKER) $(BROKER_PROCESSES)
	rm -f $(LIBS_O)
	rm -f *~
	sh ipcrma.sh

valgrind: clean $(PROGRAM)
	valgrind $(VFLAGS) ./$(PROGRAM)

.PHONY: clean


