all: bbpeer bbserver

bbpeer:
	gcc -o bbpeer bbpeer.c -lpthread -Wall

bbserver:
	gcc -o bbserver bbserver.c -Wall
clean:
	rm bbserver && rm bbpeer
