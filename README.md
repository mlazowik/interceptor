Interceptor
===========

Interceptor is a library that allows you to intercept library function calls at
runtime, by modifying GOT entries used by the PLT.

Building
--------

Use cmake >= 3.5.

Only `x86-64` is supported.

API
---

`void *intercept_function(const char *name, void *new_func);`

Redirects all calls to function named `name` that go through PLT to `new_func`.
Calls by function pointers etc. are not redirected. If multiple object expose
functions of the same name the first one to be found (in the order given by
`dl_iterate_phdr`) is redirected.

Returns the address of the original function, or `NULL` if no such function
exists.

`void unintercept_function(const char *name)`

Restores original function, or does nothing if no such function exists.

Caveats
-------

If you intercept an intercepted function the returned/restored function
will be the one before any interception has happened.

Implementation details
----------------------

An assumption is made that `.strtab` is located right after `.dynsym`.

One of the objects parsed by `dl_iterate_phdr` is `vdso`, which uses addressing
relative to its start (the addresses are not relocated). As we don't need to
read it either way it's ignored. To ignore it its address is found using
`getauxval` and is compared to current object address in `dl_iterate_phdr`
callback.

Original function address is looked up in `.dynsym` of loaded objects. Only
defined symbols are taken into consideration. Sometimes the symbol can have a
type of `STT_GNU_IFUNC`, which means that the address is not the target
function address, but an address of a function which returns the target
function. So we just invoke it and return the result.
[Read more about ifunc here](http://www.airs.com/blog/archives/403).

The GOT entries to replace are found by searching for `R_X86_64_JUMP_SLOT`
type relocations targets in `.rela.plt` (`DT_JMPREL`). This is all that has to
be done, because for the PLT mechanism this looks like a resolved function
address so it just jumps to it.
[Read more about GOT and PLT here](http://eli.thegreenplace.net/2011/11/03/position-independent-code-pic-in-shared-libraries/).