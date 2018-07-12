CC=gcc
CFLAGS=-std=c99 -g
LIBS=-lSoapySDR -lm
# DEPS = 

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

all: main.o 
	$(CC) -o limesdr_read main.o $(CFLAGS) $(LIBS)

clean:
	rm limesdr_read