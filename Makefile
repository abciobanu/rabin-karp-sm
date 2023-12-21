all: helpers rabin_karp_seq
CC=gcc-13
MPICC=mpicc
CFLAGS=

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
