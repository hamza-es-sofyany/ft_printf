#include <stdarg.h>
#include <stdio.h>

int ft_printf(const char *format, ...)
{
    va_list list;
    int i, j , k;

    va_start(list, format);
    i = 0;
    while (format[i])
    {
        if (format[i] == '%' && format[i + 1] == 'c')
        
        if (format[i] == '%' && format[i + 1] == 's')
        
        if (format[i] == '%' && format[i + 1] == 'p')
        
        if (format[i] == '%' && format[i + 1] == 'd')
        
        if (format[i] == '%' && format[i + 1] == 'i')
        
        if (format[i] == '%' && format[i + 1] == 'u')
        
        if (format[i] == '%' && format[i + 1] == 'x' || format[i + 1] == 'X')

        if (format[i] == '%' && format[i + 1] == '%')

    }
}