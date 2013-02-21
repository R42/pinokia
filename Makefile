CC = gcc -g -Wall

all: pinokia

pinokia: main.c
	$(CC) -o pinokia driver.c main.c
