# Recursive Stacks

## Purpose

`librstack.so` is a dynamically linked C library for working with recursive stacks. 
A stack can contain `uint64_t` values and references to other stacks. 
Nested stacks are shared rather than copied, so changes to a stack are visible wherever that stack is referenced. 
References are managed with a mark & sweep garbage collector, and the API supports recursive queries as well as reading and writing stack values in text files.

## Usage

Include `rstack.h` in a C source file and link the program with `librstack.so`.

The main operations are:

- **`rstack_t* rstack_new()`** — Creates an empty stack. Returns a pointer on success or `nullptr` with `errno = ENOMEM` on allocation failure.

- **`void rstack_delete(rstack_t *rs)`** — Deletes a stack. Does nothing if `rs == nullptr`. The pointer must not be used after deletion.

- **`int rstack_push_value(rstack_t *rs, uint64_t value)`** — Pushes a number onto a stack. Returns `0` on success or `-1` on failure (`EINVAL` for a null pointer, `ENOMEM` for allocation failure).

- **`int rstack_push_rstack(rstack_t *rs1, rstack_t *rs2)`** — Pushes one stack (`rs2`) onto another (`rs1`). Returns `0` on success or `-1` if either pointer is null (`EINVAL`) or allocation fails (`ENOMEM`).

- **`void rstack_pop(rstack_t *rs)`** — Removes the top element without recursion. Does nothing if the pointer is null or the stack is empty.

- **`bool rstack_empty(rstack_t *rs)`** — Recursively checks whether a stack contains a number. Returns `true` if the pointer is null or the stack is empty; otherwise, returns `false`.

- **`result_t rstack_front(rstack_t *rs)`** — Recursively finds the number closest to the stack's top. Returns `{flag = true, value = number}` if found; otherwise, `flag = false` if the pointer is null, the stack is empty, or no number is found.

  ```cpp
  typedef struct {
      bool flag;
      uint64_t value;
  } result_t;
  ```

- **`rstack_t* rstack_read(char const *path)`** — Creates a stack from a file containing base-10 numbers separated by whitespace. Leading, trailing, and repeated whitespace are allowed. Validates the entire file. Returns a stack pointer on success or `nullptr` on error, setting `errno` appropriately.

- **`int rstack_write(char const *path, rstack_t *rs)`** — Writes stack numbers to a file, one per line in base 10, without leading zeros or extra whitespace. Every line ends with `\n`. Returns `0` on success or `-1` on error, setting `errno` appropriately. Writing is aborted if a cycle is detected.

For `rstack_read()` and `rstack_write()`, selecting appropriate `errno` values is part of the implementation requirements.

Operations that can fail report errors using their documented return value and `errno`.

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

## Acknowledgements

The tests used to validate this implementation as well as the header file outline were provided by the staff of University of Warsaw.
