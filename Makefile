RM = rm -rf
CC = gcc
CFLAGS = -std=c99 -g -Werror -Wall -pedantic -D_POSIX_C_SOURCE=200809L
LDFLAGS= -pthread

default: all

all: bin client 

bin:
	mkdir -p bin/

client: cloudflare.c
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o bin/cloudflare

clean:
	$(RM) bin/

.PHONY: client clean
