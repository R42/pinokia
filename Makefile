CFLAGS = -g -Wall

.PHONY: clean

all: pinokia sendword

clean:
	rm pinokia *.o

pinokia: main.o driver.o modules.o

sendword: sendword.o modules.o

it: all
run: all
	sudo ./pinokia
