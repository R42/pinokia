CC = gcc -g -Wall

.PHONY: clean

all: pinokia

clean:
	rm pinokia

pinokia: main.c driver.c
	$(CC) -o pinokia driver.c main.c

it: all
run: all
	sudo ./pinokia
