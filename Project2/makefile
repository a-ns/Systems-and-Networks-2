all: network receiver sender

network:
	gcc -o network network.c checksum.c -lpthread -Wall -g

sender:
	gcc -o sender sender.c checksum.c -Wall -g

receiver:
	gcc -o receiver receiver.c checksum.c -Wall -g -lpthread
clean:
	rm network && rm sender && rm receiver
