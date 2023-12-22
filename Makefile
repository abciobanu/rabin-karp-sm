all: build_helpers build_rabin_karp_seq build_rabin_karp_openmp build_rabin_karp_pthreads build_rabin_karp_mpi test_seq test_openmp test_pthreads test_mpi test_mpi_openmp
CC=gcc
MPICC=mpicc
CFLAGS=-std=gnu99 -Wall -Wextra

NUM_MPI_PROCESSES := 8

TESTS_DIR := tests
NUM_TESTS := 10

# Helpers
HELPERS := helpers.c

# Sequential Rabin-Karp
SEQ_RABIN_KARP := rabin_karp_seq.c

# OpenMP Rabin-Karp
OPENMP_RABIN_KARP := rabin_karp_openmp.c

# PThreads Rabin-Karp
PTHREADS_RABIN_KARP := rabin_karp_pthreads.c

# MPI Rabin-Karp
MPI_RABIN_KARP := rabin_karp_mpi.c

# MPI + OpenMP Rabin-Karp
MPI_OPENMP_RABIN_KARP := rabin_karp_mpi_openmp.c

build_helpers: $(HELPERS)
	$(CC) -c $(HELPERS) -o helpers.o $(CFLAGS)

build_rabin_karp_seq: $(HELPERS) $(SEQ_RABIN_KARP)
	$(CC) $(HELPERS) $(SEQ_RABIN_KARP) -o rabin_karp_seq $(CFLAGS)

build_rabin_karp_openmp: $(HELPERS) $(OPENMP_RABIN_KARP)
	$(CC) $(HELPERS) $(OPENMP_RABIN_KARP) -o rabin_karp_openmp $(CFLAGS) -fopenmp

build_rabin_karp_pthreads: $(HELPERS) $(PTHREADS_RABIN_KARP)
	$(CC) $(HELPERS) $(PTHREADS_RABIN_KARP) -o rabin_karp_pthreads $(CFLAGS) -lpthread

build_rabin_karp_mpi: $(HELPERS) $(MPI_RABIN_KARP)
	$(MPICC) $(HELPERS) $(MPI_RABIN_KARP) -o rabin_karp_mpi $(CFLAGS)

build_rabin_karp_mpi_openmp: $(HELPERS) $(MPI_OPENMP_RABIN_KARP)
	$(MPICC) $(HELPERS) $(MPI_OPENMP_RABIN_KARP) -o rabin_karp_mpi_openmp $(CFLAGS) -fopenmp

test_seq: build_rabin_karp_seq
	@echo "Testing sequential Rabin-Karp algorithm..."
	time ./rabin_karp_seq $(TESTS_DIR) $(NUM_TESTS);

test_openmp: build_rabin_karp_openmp
	@echo "Testing OpenMP Rabin-Karp algorithm..."
	time ./rabin_karp_openmp $(TESTS_DIR) $(NUM_TESTS);

test_pthreads: build_rabin_karp_pthreads
	@echo "Testing PThreads Rabin-Karp algorithm..."
	time ./rabin_karp_pthreads $(TESTS_DIR) $(NUM_TESTS);

test_mpi: build_rabin_karp_mpi
	@echo "Testing MPI Rabin-Karp algorithm..."
	time mpirun -np $(NUM_MPI_PROCESSES) ./rabin_karp_mpi $(TESTS_DIR) $(NUM_TESTS);

test_mpi_openmp: build_rabin_karp_mpi_openmp
	@echo "Testing MPI + OpenMP Rabin-Karp algorithm..."
	time mpirun -np $(NUM_MPI_PROCESSES) ./rabin_karp_mpi_openmp $(TESTS_DIR) $(NUM_TESTS);

clean:
	@rm -f helpers.o rabin_karp_seq rabin_karp_openmp rabin_karp_pthreads rabin_karp_mpi rabin_karp_mpi_openmp

.PHONY: all clean
