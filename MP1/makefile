all: crc crsd

clean: 
	rm -f *.o *.bin crc

crc.o: crc.c
	g++ -c crc.c -pthread

crc: crc.o
	g++ crc.o -o crc -pthread

crsd.o: crsd.c 
	g++ -c crsd.c -pthread

crsd: crsd.o
	g++ crsd.o -o crsd -pthread