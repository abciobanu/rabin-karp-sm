# HW 2 - SM

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


### Tests
* `tests` are generated with the default parameters of `test_cooker.py`.
* `large_tests` are generated like this:
```bash
python test_cooker.py --max_test=20 --min_text=10000 --max_text=100000 --min_pattern=10 --max_pattern=200 -o large_tests
```
