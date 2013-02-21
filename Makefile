CC = gcc -g -Wall

all: pinokia

clean:
	rm pinokia

pinokia: main.c driver.c
	$(CC) -o pinokia driver.c main.c
