all: crc crsd temp

clean: 
	rm -f *.o *.bin crc

crc.o: crc.c
	g++ -c crc.c

crc: crc.o
	g++ crc.o -o crc

crsd.o: crsd.c 
	g++ -c crsd.c

crsd: crsd.o
	g++ crsd.o -o crsd

temp.o: temp.c 
	g++ -c temp.c

temp: temp.o
	g++ temp.o -o temp