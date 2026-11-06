*This project has been created as part of the 42 curriculum by hes-sofy.*

# ft_printf

## Description

`ft_printf` is a C library that recreates the mandatory subset of the standard C `printf()` function required by the 42 `ft_printf` project (subject version 12.1).

The goal is to parse a format string, retrieve the corresponding arguments from a variable-length argument list, and write the formatted output to standard output. The implementation uses `write()` for output and handles the following mandatory conversions:

| Conversion | Description |
| --- | --- |
| `%c` | Writes one character. |
| `%s` | Writes a string. |
| `%p` | Writes a pointer address in hexadecimal form. |
| `%d` | Writes a signed decimal integer. |
| `%i` | Writes a signed decimal integer. |
| `%u` | Writes an unsigned decimal integer. |
| `%x` | Writes an unsigned hexadecimal integer using lowercase letters. |
| `%X` | Writes an unsigned hexadecimal integer using uppercase letters. |
| `%%` | Writes a literal percent sign. |

The function prototype is:

```c
int ft_printf(const char *format, ...);
```

The return value is the number of characters written, following the behaviour expected by the project.

This repository covers the **mandatory part**. Bonus features such as field width, precision, and the `-`, `0`, `.`, `#`, `+`, and space flags are not implemented as part of this version.

## Project structure

```text
ft_printf/
├── Makefile
├── ft_printf.h
├── ft_printf.c
├── ft_printf_char.c
├── ft_printf_hex.c
├── ft_printf_number.c
├── ft_printf_pointer.c
└── tests/
    └── ft_printf_500_tests.c
```

The `tests/` directory contains an optional test runner. It is separate from the library and is not needed to build `libftprintf.a`.

## Instructions

### Requirements

- A C compiler (`cc`).
- `make`.
- `ar` to create the static library.

### Build the library

Run the following command from the repository root:

```bash
make
```

This builds the static library:

```text
libftprintf.a
```

The library is created at the root of the repository, as required by the subject.

### Use the library in a C program

Include the project header:

```c
#include "ft_printf.h"

int main(void)
{
    ft_printf("Hello %s! Number: %d\n", "1337", 42);
    return (0);
}
```

Compile and link your program with the library:

```bash
cc -Wall -Wextra -Werror -I. example.c libftprintf.a -o example
```

Run it:

```bash
./example
```

### Makefile rules

The Makefile provides the required rules:

| Rule | Action |
| --- | --- |
| `make` / `make all` | Builds `libftprintf.a`. |
| `make clean` | Removes object files. |
| `make fclean` | Removes object files and `libftprintf.a`. |
| `make re` | Performs a full rebuild. |

### Run the 500-test suite

The repository's additional test runner can be used to compare `ft_printf()` against the system `printf()` for the included test cases. It checks the emitted bytes and return values and includes boundary, mixed-conversion, long-string, and stress cases.

Place the test source at `tests/ft_printf_500_tests.c`, then build the library with AddressSanitizer and UndefinedBehaviorSanitizer enabled:

```bash
make fclean
make CFLAGS="-Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer"
```

Compile and run the test runner:

```bash
cc -Wall -Wextra -Werror -g -fsanitize=address,undefined \
   -fno-omit-frame-pointer -I. tests/ft_printf_500_tests.c \
   libftprintf.a -o test500

./test500
```

The test suite is an additional validation tool, not an official 42 correction or a mathematical proof that every possible input is error-free. In the recorded test run for this project, all 500 cases passed, with no reported failures, crashes, or timeouts.

## Algorithm and data structures

### Format-string processing

`ft_printf()` processes the format string from left to right:

1. It initializes a `va_list` with `va_start()` to access the arguments supplied after the format string.
2. It scans the format string one character at a time.
3. When the current character is not `%`, it writes that character to standard output.
4. When it encounters `%`, it examines the following character and dispatches to the helper responsible for that conversion.
5. The helper retrieves the next argument using `va_arg()` with the type required by the conversion, then writes its representation.
6. After processing the format string, `va_end()` is called and the function returns the number of characters written.

This sequential parsing strategy is sufficient for the mandatory format because it only needs to recognise the nine specified conversion forms. It does not need to build a complete format description or reproduce the buffering system of the original `printf()`.

### Conversion helpers

The implementation separates conversion logic into helper source files:

- `ft_printf_char.c` handles character and string output (`%c` and `%s`).
- `ft_printf_pointer.c` handles pointer output (`%p`).
- `ft_printf_number.c` handles signed and unsigned decimal output (`%d`, `%i`, and `%u`).
- `ft_printf_hex.c` handles hexadecimal output (`%x` and `%X`).
- `ft_printf.c` coordinates format parsing and conversion dispatch.

Keeping conversion logic in small, focused helpers avoids placing every conversion in one large function. It also makes each conversion easier to test and maintain.

### Variadic arguments

A `va_list` is used to traverse the arguments after `format`. Each conversion must retrieve its argument using the correct type:

| Conversion | Argument type retrieved |
| --- | --- |
| `%c` | `int` |
| `%s` | `char *` |
| `%p` | `void *` |
| `%d`, `%i` | `int` |
| `%u`, `%x`, `%X` | `unsigned int` |
| `%%` | No argument |

Using the correct type is essential because variadic functions do not receive type information at runtime. Reading an argument with an incompatible type can cause undefined behaviour.

### Data structures and design choices

The mandatory implementation does not require a complex data structure. It uses:

- The format-string pointer (or its current index) to track the character being parsed.
- `va_list` to track the next variadic argument.
- Local integer, character, and pointer variables inside the conversion helpers.

This is appropriate because the format is processed sequentially and each conversion can be handled as soon as it is encountered. A token array, linked list, or dynamically allocated format representation would add complexity without being necessary for the required conversions.

The library writes output directly with `write()` rather than attempting to reproduce libc's internal output-buffer management, which the subject explicitly excludes.

## Resources

- The official 42 `ft_printf` subject, version 12.1.
- [`printf(3)` — Linux manual page](https://man7.org/linux/man-pages/man3/printf.3.html): reference for the standard formatted-output function and conversion behaviour.
- [`stdarg(3)` — Linux manual page](https://man7.org/linux/man-pages/man3/stdarg.3.html): reference for `va_list`, `va_start`, `va_arg`, and `va_end`.
- [`write(2)` — Linux manual page](https://man7.org/linux/man-pages/man2/write.2.html): reference for writing bytes to a file descriptor.
- [C variadic arguments — cppreference](https://en.cppreference.com/w/c/variadic.html): an additional explanation of variadic functions in C.

### AI usage

AI assistance was used as a learning and testing support tool during this project. It helped clarify variadic-function concepts and conversion edge cases, discuss test coverage, generate an additional 500-case test runner, and organise this documentation. The test runner was compiled and executed against this implementation. AI assistance was not treated as proof of correctness; the project author is responsible for understanding the implementation, checking the generated tests, and being able to explain the code and design choices during evaluation.

## Author

- **42 login:** `hes-sofy`
