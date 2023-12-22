all: helpers rabin_karp_seq rabin_karp_openmp rabin_karp_pthreads
CC=gcc
MPICC=mpicc
CFLAGS=-std=gnu99 -Wall -Wextra

# Helpers
HELPERS := helpers.c

# Sequential Rabin-Karp
SEQ_RABIN_KARP := rabin_karp_seq.c

# OpenMP Rabin-Karp
OPENMP_RABIN_KARP := rabin_karp_openmp.c

# PThreads Rabin-Karp
PTHREADS_RABIN_KARP := rabin_karp_pthreads.c

helpers: $(HELPERS)
	$(CC) -c $(HELPERS) -o helpers.o $(CFLAGS)

rabin_karp_seq: $(HELPERS) $(SEQ_RABIN_KARP)
	$(CC) $(HELPERS) $(SEQ_RABIN_KARP) -o rabin_karp_seq $(CFLAGS)

rabin_karp_openmp: $(HELPERS) $(OPENMP_RABIN_KARP)
	$(CC) $(HELPERS) $(OPENMP_RABIN_KARP) -o rabin_karp_openmp $(CFLAGS) -fopenmp

rabin_karp_pthreads: $(HELPERS) $(PTHREADS_RABIN_KARP)
	$(CC) $(HELPERS) $(PTHREADS_RABIN_KARP) -o rabin_karp_pthreads $(CFLAGS) -lpthread

clean:
	@rm -f helpers.o rabin_karp_seq rabin_karp_openmp rabin_karp_pthreads

.PHONY: all clean
