CFLAGS = -g -Wall

.PHONY: clean

all: pinokia sendword spidev_test

clean:
	rm pinokia *.o

modules.o: modules.c

driver.o: driver.c

main.o: main.c

sendword.o: sendword.c

pinokia: main.o driver.o modules.o
	$(CC) $(CFLAGS) -o $@ main.o driver.o modules.o

sendword: sendword.o modules.o
	$(CC) $(CFLAGS) -o $@ sendword.o modules.o

it: all
run: all
	sudo ./pinokia
