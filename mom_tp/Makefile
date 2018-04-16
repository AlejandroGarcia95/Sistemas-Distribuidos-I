CFLAGS := -g -std=c99
VFLAGS := --leak-check=full --show-leak-kinds=all --track-origins=yes
LIBS_PATH = libs
LIBS_C = $(wildcard $(LIBS_PATH)/*.c)
LIBS_O = $(patsubst $(LIBS_PATH)/%.c, $(LIBS_PATH)/%.o, $(LIBS_C))
SRCS = mom.o
PROGRAM = user_main
DAEMON_PROCESSES = mom_responser mom_requester
MOM_DAEMON = mom_daemon


# ----------------------------------------------------------------------

all: clean $(PROGRAM) $(DAEMON_PROCESSES) $(MOM_DAEMON)


$(PROGRAM): $(LIBS_O) $(SRCS) $(PROGRAM).c
	gcc -o $(PROGRAM) $^

$(MOM_DAEMON): $(LIBS_O) $(MOM_DAEMON).c
	gcc -o $(MOM_DAEMON) $^

mom_requester: $(LIBS_O) mom_requester.c
	gcc -o mom_requester $^

mom_responser: $(LIBS_O) mom_responser.c
	gcc -o mom_responser $^
	
run: clean $(PROGRAM) $(DAEMON_PROCESSES) $(MOM_DAEMON)
	./$(PROGRAM)


%.o: %.c
	gcc $(CFLAGS) -c $< -o $@ 

clean:
	rm -f $(PROGRAM) *.o *.temp $(MOM_DAEMON) $(DAEMON_PROCESSES)
	rm -f $(LIBS_O)
	rm -f *~
	sh ipcrma.sh

valgrind: clean $(PROGRAM)
	valgrind $(VFLAGS) ./$(PROGRAM)

.PHONY: clean


