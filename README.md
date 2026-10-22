# Recursive Stacks

## Purpose

`librstack.so` is a dynamically linked C library for working with recursive stacks. 
A stack can contain `uint64_t` values and references to other stacks. 
Nested stacks are shared rather than copied, so changes to a stack are visible wherever that stack is referenced. 
References are managed with a mark & sweep garbage collector, and the API supports recursive queries as well as reading and writing stack values in text files.

## Usage

Include `rstack.h` in a C source file and link the program with `librstack.so`.

The main operations are:

- `rstack_new()` creates an empty stack.
- `rstack_delete(rs)` releases a reference to a stack. Do not use a pointer after its stack has been freed.
- `rstack_push_value(rs, value)` pushes a `uint64_t` value.
- `rstack_push_rstack(rs1, rs2)` pushes a reference to `rs2` onto `rs1`; it does not copy `rs2`.
- `rstack_pop(rs)` removes the top item, if present.
- `rstack_empty(rs)` checks recursively whether any value is reachable from the stack.
- `rstack_front(rs)` returns a `result_t`; `flag` indicates whether `value` contains the nearest reachable value.
- `rstack_read(path)` creates a stack from decimal numbers separated by whitespace in a text file.
- `rstack_write(path, rs)` writes reachable values as decimal numbers, one per line. Writing is stopped if a cycle is detected.

Operations that can fail report errors using their documented return value and `errno`. For example, push operations return `0` on success and `-1` on failure; `rstack_read()` returns `NULL` on failure.

## Building

On Linux, with GCC and GNU Make installed, build the shared library from the project directory:

```sh
make librstack.so
```

This creates `librstack.so` in the current directory.

Compile the testing program with the library:

```sh
make example
```

Run tests with

```sh
./rstack_example <test_name>
```

Remove generated build files with:

```sh
make clean
```
