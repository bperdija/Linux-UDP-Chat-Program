#compile and link the application
all: lets-talk

#run the application + valgrind
run1: lets-talk
	./lets-talk	3000 127.0.0.1 3001

run2: lets-talk
	./lets-talk 3001 127.0.0.1 3000

lets-talk: lets-talk.c list.c
	gcc -g -pthread -o lets-talk lets-talk.c list.c 

#remove built files
clean:
	rm -rf lets-talk
