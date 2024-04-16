all: TCP_Sender TCP_Receiver RUDP_Sender RUDP_Receiver

Sender.o: TCP_Sender.c TCP_Sender.h
	gcc -c Sender.c

Receiver.o: TCP_Receiver.c TCP_Receiver.h
	gcc -c TCP_Receiver.c

Sender: TCP_Sender.o
	gcc -o TCP_Sender TCP_Sender.o

Receiver: TCP_Receiver.o
	gcc -o TCP_Receiver TCP_Receiver.o

RUDP_Sender.o: RUDP_Sender.c RUDP.c RUDP_API.h
	gcc -c RUDP_Sender.c RUDP.c

RUDP_Receiver.o: RUDP_Receiver.c RUDP.c RUDP_API.h
	gcc -c RUDP_Receiver.c RUDP.c

RUDP.o: RUDP.c RUDP_API.h
	gcc -c RUDP.c

RUDP_Sender: RUDP_Sender.o RUDP.o
	gcc -o RUDP_Sender RUDP_Sender.o RUDP.o

RUDP_Receiver: RUDP_Receiver.o RUDP.o
	gcc -o RUDP_Receiver RUDP_Receiver.o RUDP.o


clean:
	rm -f *.o TCP_Sender TCP_Receiver RUDP_Sender RUDP_Receiver