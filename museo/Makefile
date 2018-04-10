CFLAGS := -g
VFLAGS := --leak-check=full --show-leak-kinds=all --track-origins=yes
ARCHIVOS = argv_parser.o
PROGRAMA = main door person guide tour_timeout

all: clean $(PROGRAMA)

main: $(ARCHIVOS) main.o
	gcc -o main $^

door: $(ARCHIVOS) door.o
	gcc -o door $^

tour_timeout: $(ARCHIVOS) tour_timeout.o
	gcc -o tour_timeout $^


guide: $(ARCHIVOS) guide.o
	gcc -o guide $^

person: $(ARCHIVOS) person.o
	gcc -o person $^
	
run: clean $(PROGRAMA)
	./main


%.o: %.c
	gcc $(CFLAGS) -c $< -o $@ 

clean:
	rm -f $(PROGRAMA) *.o *.temp
	sh ipcrma.sh

valgrind: clean $(PROGRAMA)
	valgrind $(VFLAGS) ./$(PROGRAMA)

.PHONY: clean


