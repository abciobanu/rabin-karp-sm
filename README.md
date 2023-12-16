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

