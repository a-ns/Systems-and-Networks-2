all: bbpeer bbserver

bbpeer:
	gcc -o bbpeer bbpeer.c -lpthread

bbserver:
	gcc -o bbserver bbserver.c -lpthread
clean:
	rm bbserver && rm bbpeer
