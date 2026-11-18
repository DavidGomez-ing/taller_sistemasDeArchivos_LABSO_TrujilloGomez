CC     = gcc
CFLAGS = -Wall -Wextra -O2

infoarchivo: infoarchivo.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f infoarchivo

.PHONY: clean
