all: TCP_Sender TCP_Receiver

Sender.o: TCP_Sender.c TCP_Sender.h
	gcc -c Sender.c

Receiver.o: TCP_Receiver.c TCP_Receiver.h
	gcc -c TCP_Receiver.c

Sender: TCP_Sender.o
	gcc -o TCP_Sender TCP_Sender.o

Receiver: TCP_Receiver.o
	gcc -o TCP_Receiver TCP_Receiver.o

clean:
	rm -f *.o TCP_Sender TCP_Receiver