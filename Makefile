CC = gcc
CFLAGS = -O3 -lm -lpthread
PREFIX = /usr/local

all:
	$(CC) -o cybervis cybervis.c $(CFLAGS)

install: all
	install -m 755 cybervis $(PREFIX)/bin/cybervis

uninstall:
	rm -f $(PREFIX)/bin/cybervis

clean:
	rm -f cybervis
