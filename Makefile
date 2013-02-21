CC = gcc -g -Wall

all: pinokia

clean:
	rm pinokia

pinokia: main.c
	$(CC) -o pinokia driver.c main.c
