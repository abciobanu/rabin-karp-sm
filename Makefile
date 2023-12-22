all: helpers rabin_karp_seq
CC=gcc
MPICC=mpicc
CFLAGS=-std=gnu99 -Wall -Wextra

# Helpers
HELPERS := helpers.c

# Sequential Rabin-Karp
SEQ_RABIN_KARP := rabin_karp_seq.c

helpers: $(HELPERS)
	$(CC) -std=c11 -c $(HELPERS) -o helpers.o $(CFLAGS)

rabin_karp_seq: $(HELPERS) $(SEQ_RABIN_KARP)
	$(CC) -std=c11 $(HELPERS) $(SEQ_RABIN_KARP) -o rabin_karp_seq $(CFLAGS)

clean:
	@rm -f helpers.o rabin_karp_seq

.PHONY: all clean
