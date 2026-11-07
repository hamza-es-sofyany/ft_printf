*This project has been created as part of the 42 curriculum by hes-sofy.*

# ft_printf

## Description

The **ft_printf** project consists of recreating the standard C `printf()` function from the C standard library.

The main goal is to understand how variadic functions work in C and how to handle different data types and format specifiers.

The implementation uses the `write()` function to print formatted output directly to the standard output.

### Supported Conversions

| Conversion | Description |
|:---:|---|
| `%c` | Prints a single character. |
| `%s` | Prints a string. |
| `%p` | Prints a pointer address in hexadecimal format. |
| `%d` | Prints a signed decimal integer. |
| `%i` | Prints an integer in base 10. |
| `%u` | Prints an unsigned decimal integer. |
| `%x` | Prints a hexadecimal number using lowercase letters. |
| `%X` | Prints a hexadecimal number using uppercase letters. |
| `%%` | Prints the percent character `%`. |

The function returns the total number of characters printed.

## Instructions

### Compilation

To compile the library, run:

```bash
make
```

This generates the static library:

```text
libftprintf.a
```

### Makefile Commands

| Command | Description |
|---|---|
| `make` | Compiles the source files and creates the library. |
| `make clean` | Removes the object files (`.o`). |
| `make fclean` | Removes the object files and the library. |
| `make re` | Cleans and recompiles the library. |

### Usage

First, include the project header in your C file:

```c
#include "ft_printf.h"
```

You can then use `ft_printf()` in your program.

Example:

```c
#include "ft_printf.h"

int main(void)
{
    ft_printf("Hello, %s!\n", "World");
    ft_printf("Number: %d\n", 42);
    ft_printf("Hexadecimal: %x\n", 255);
    return (0);
}
```

### Compile Your Program

After writing your C file, compile it with the library:

```bash
cc -Wall -Wextra -Werror main.c libftprintf.a
```

The command uses:

- `-Wall`: Enables common warnings.
- `-Wextra`: Enables additional warnings.
- `-Werror`: Treats warnings as errors.
- `main.c`: Your C source file.
- `libftprintf.a`: The ft_printf library.

### Run Your Program

```bash
./a.out
```

## Algorithm and Technical Choices

### 1. Format String Parsing

The main function, `ft_printf()`, reads the format string character by character.

It follows this logic:

1. If the current character is not `%`, it prints that character directly.
2. If it encounters `%`, it reads the next character as a conversion specifier.
3. It passes the specifier to `ft_eval()`.
4. `ft_eval()` identifies the conversion and calls the appropriate helper function.

This approach separates format parsing from the actual printing operations and keeps the code organized.

### 2. Variadic Arguments

The project uses the `stdarg.h` library to handle a variable number of arguments.

The following macros are used:

- `va_start()`: Initializes the argument list.
- `va_arg()`: Retrieves the next argument with its expected data type.
- `va_end()`: Ends the argument list.

Each conversion retrieves its argument according to the required C type.

| Conversion | Argument type |
|:---:|---|
| `%c` | `int` |
| `%s` | `char *` |
| `%p` | `void *` |
| `%d` | `int` |
| `%i` | `int` |
| `%u` | `unsigned int` |
| `%x` | `unsigned int` |
| `%X` | `unsigned int` |
| `%%` | No argument |

### 3. Character and String Handling

The `ft_print_char()` function prints one character using `write()`.

The `ft_print_string()` function iterates through a string and prints its characters one by one.

If the string pointer is `NULL`, the implementation prints `(null)`.

### 4. Number Conversion Algorithm

The project handles decimal and hexadecimal numbers using division and modulo operations.

The general algorithm is:

1. Extract the last digit using the modulo operator.
2. Store the digit in a temporary character array.
3. Remove the last digit using integer division.
4. Repeat until the number has been fully processed.
5. Print the stored digits in reverse order.

For decimal numbers, the base is 10.

For hexadecimal numbers, the base is 16. A character string containing hexadecimal digits is used to select the correct character for each digit.

For signed integers, the implementation uses a `long` variable to handle the negative value of `INT_MIN` safely during conversion.

### 5. Pointer Conversion

The `%p` conversion prints a pointer address in hexadecimal format.

For a non-NULL pointer, the implementation prints the `0x` prefix followed by the hexadecimal address.

For a NULL pointer, it prints `(nil)`.

### 6. Data Structures and Memory Management

The implementation does not require complex data structures.

It mainly uses:

- `va_list` to manage the variable arguments.
- Local character arrays to temporarily store digits.
- Integer variables to track positions and count printed characters.

No dynamic memory allocation is required for the mandatory conversions implemented in this project.

### 7. Output and Return Value

The project uses the `write()` system call to print characters directly to the standard output.

The helper functions return the number of characters they print, and `ft_printf()` accumulates these values.

At the end, `ft_printf()` returns the total number of characters printed.

The implementation does not reproduce the internal buffer management of the original `printf()`, as required by the project subject.

## Resources

### Documentation and References

- [printf(3) — Linux Manual Page](https://man7.org/linux/man-pages/man3/printf.3.html)
- [stdarg.h — C Variadic Arguments](https://en.cppreference.com/w/c/variadic.html)
- [write(2) — Linux Manual Page](https://man7.org/linux/man-pages/man2/write.2.html)
- [C Language Documentation — cppreference](https://en.cppreference.com/w/c.html)

### AI Usage

AI tools were used as learning and support resources throughout the project.

They helped with:

- Understanding variadic functions and the `stdarg.h` macros.
- Clarifying the behavior of format specifiers and their argument types.
- Understanding implementation logic and debugging problems.
- Discussing edge cases and improving the project documentation.

AI was used to support the learning and development process, rather than as a replacement for understanding the code and its implementation.