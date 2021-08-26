CC = gcc
OPT = -O2
STD = -std=c11
CFLAGS = $(OPT) $(STD) -c -Wall -pedantic

OBJ = main.o

nmea: $(OBJ)
	$(CC) $(OBJ) -o $@

main.o: main.c
	$(CC) $(CFLAGS) main.c

clean:
	-rm *.o
	-rm nmea