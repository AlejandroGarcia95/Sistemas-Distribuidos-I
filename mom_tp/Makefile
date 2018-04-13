CFLAGS := -g -std=c99
VFLAGS := --leak-check=full --show-leak-kinds=all --track-origins=yes
LIBS_PATH = libs
LIBS_C = $(wildcard $(LIBS_PATH)/*.c)
LIBS_O = $(patsubst $(LIBS_PATH)/%.c, $(LIBS_PATH)/%.o, $(LIBS_C))
SRCS = mom.o
PROGRAM = user_main



# ----------------------------------------------------------------------

all: clean $(PROGRAM)
	
pepe:
	echo $(SRCS_O)

user_main: $(LIBS_O) $(SRCS) $(PROGRAM).c
	gcc -o $(PROGRAM) $^

	
run: clean $(PROGRAM)
	./$(PROGRAM)


%.o: %.c
	gcc $(CFLAGS) -c $< -o $@ 

clean:
	rm -f $(PROGRAM) *.o *.temp
	rm -f $(LIBS_O)
	sh ipcrma.sh

valgrind: clean $(PROGRAM)
	valgrind $(VFLAGS) ./$(PROGRAM)

.PHONY: clean


