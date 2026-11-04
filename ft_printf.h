#ifndef FT_PRINTF_H
# define FT_PRINTF_H

# include <unistd.h>
# include <stdlib.h>

int		ft_printf(const char *format, ...);


int		ft_print_char(int c);
int		ft_print_string(const char *s);


int		ft_print_signed(int n);
int		ft_print_unsigned(unsigned int n);
int	ft_strlen(const char *s);

int		ft_print_pointer(void *ptr);


int		ft_print_hex(unsigned int n, char specifier);


int		ft_write_char(char c);
int		ft_write_string(const char *s);
int		ft_putnbr_base(unsigned long long nbr, const char *base);

#endif