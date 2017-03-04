all: network receiver sender

network:
	gcc -o network network.c -lpthread -Wall -g

sender:
	gcc -o sender sender.c -Wall -g

receiver:
	gcc -o receiver receiver.c -Wall -g
clean:
	rm bbserver && rm bbpeer
