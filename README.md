# HW 2 - SM

**Team members:** Andrei-Bogdan Ciobanu (341C1) & Radu-Andrei Dumitru (341C1).


### `test_cooker.py`
* A lot of parameters can be modified by providing arguments to the command
(check the main function, the parameters are described by the help field).
* The text generator is really simple, but the pattern generator has some catches.
It tries to generate some disjoint intervals, where to insert the patterns in
the text.
The purpose is to not write two or more patterns in the same place.
This, of course, has some limitations because the space range is not that large,
but it can be adjusted.
If the space range search fails, then the program won't just loop because it 
will be stopped by the timeout handler. Obviously, some automatic retries can
be done in function `main`, but, for now, a manual try is enough, the program
will show relevant output in case of failure.
* The characters for the text generation can be altered by modifying this line:
```python
return ''.join(choices(ascii_letters + ' ', k=length))
```
(just add special characters to `ascii_letters + ' '`)


### OpenMP
* The parallelization using OpenMP was very easy to implement starting from
the sequential one. Just by adding some parallel fors, the program was significantly
speeded up.
* Also, `#pragma omp critical` was needed for avoiding a data race.

### PThreads
* More quicker than OpenMP implementation, but obviously not so pretty, because
of nested threads.
Firstly, the array of patterns was splitted, each thread having its own range.
Secondly, each thread will launch. for each search, multiple threads.
* The nice part comes to synchronization, only a mutex was needed for adding to
the indexes array.

### MPI
* The programming model used for this implementation follows the same pattern used
in [the APP project](https://gitlab.cs.pub.ro/app-2023/huffmanescu). For both that
project and this one, the implementation with MPI was written entirely by me, Andrei Ciobanu.
* Slower than the other two implementations, but faster than the sequential one.
* The algorithm has three kinds of processes:
    * MAPPER (MASTER) process
        * Its role is to send tasks to the workers (mapping step) and then
        receive the aggregated results from the reducer process.
        * A task is a test case, which contains the text and the patterns to be
        searched.
        * It communicates with the reducer process in order to send/receive worker
        availability updates (if there are more tasks than workers, such updates
        are needed).
        * It checks the correctness of the results.
    * REDUCER process
        * It receives the results from the workers and aggregates them.
        * It communicates with the master process in order to send/receive worker
        availability updates.
    * WORKER processes
        * They receive tasks from the master process and send the results to the
        reducer process.
        * They do the hard work - the actual search.
* The main hassle was the communication between processes, because the input and
output payloads are not trivial, so there is plenty of data to be sent/received.

### MPI + OpenMP
* The same as MPI, but the search is parallelized using OpenMP (similar to the pure
OpenMP implementation).
* Inherits the same development issues as the two implementations.


### Tests
* `tests` are generated with the default parameters of `test_cooker.py`.
* `large_tests` are generated like this:
```bash
python test_cooker.py --max_test=20 --min_text=10000 --max_text=100000 --min_pattern=10 --max_pattern=200 -o large_tests
```


### Compiling and running
* `make` will compile all the implementations.
* `make run` will run all the implementations on the `tests` directory.


### Test run
This test run was performed with a test directory generated with the following command:
```bash
python3 test_cooker.py --min_text=5000000 --max_text=10000000
```
The test directory contains 10 tests, each with a text of 5-10 million characters
(5-10 MB), 1-10 patterns (each pattern has 10-80 characters), and 0-10 occurrences
of each pattern. The resulting directory has a size of 64 MB.

```bash
Testing sequential Rabin-Karp algorithm...
time ./rabin_karp_seq tests 10;
test 0: PASSED
test 1: PASSED
test 2: PASSED
test 3: PASSED
test 4: PASSED
test 5: PASSED
test 6: PASSED
test 7: PASSED
test 8: PASSED
test 9: PASSED

real	1m27.270s
user	1m26.976s
sys	0m0.068s


Testing OpenMP Rabin-Karp algorithm...
time ./rabin_karp_openmp tests 10;
test 6: PASSED
test 5: PASSED
test 4: PASSED
test 3: PASSED
test 0: PASSED
test 2: PASSED
test 9: PASSED
test 1: PASSED
test 8: PASSED
test 7: PASSED

real	0m18.843s
user	1m37.270s
sys	0m0.121s


Testing PThreads Rabin-Karp algorithm...
time ./rabin_karp_pthreads tests 10;
test 0: PASSED
test 1: PASSED
test 2: PASSED
test 3: PASSED
test 4: PASSED
test 5: PASSED
test 6: PASSED
test 7: PASSED
test 8: PASSED
test 9: PASSED

real	0m7.918s
user	1m50.816s
sys	0m0.165s


Testing MPI Rabin-Karp algorithm...
time mpirun -np 8 ./rabin_karp_mpi tests 10;
test 0: PASSED
test 1: PASSED
test 2: PASSED
test 3: PASSED
test 4: PASSED
test 5: PASSED
test 6: PASSED
test 7: PASSED
test 8: PASSED
test 9: PASSED

real	0m47.628s
user	4m50.670s
sys	0m2.471s
```

### References
* [Rabin–Karp algorithm](https://en.wikipedia.org/wiki/Rabin%E2%80%93Karp_algorithm)
* [Rabin fingerprint](https://en.wikipedia.org/wiki/Rabin_fingerprint)
* [Huffmanescu](https://gitlab.cs.pub.ro/app-2023/huffmanescu)
