/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hes-sofy <hes-sofy@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/05 17:19:03 by hes-sofy          #+#    #+#             */
/*   Updated: 2026/11/07 10:31:03 by hes-sofy         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_PRINTF_H
# define FT_PRINTF_H

# include <stdarg.h>
# include <stdlib.h>
# include <unistd.h>

int	ft_printf(const char *format, ...);

int	ft_print_char(int c);
int	ft_print_string(const char *s);

int	ft_print_signed(int n);
int	ft_print_unsigned(unsigned int n);

int	ft_print_pointer(void *ptr);

int	ft_print_hex(unsigned int n, char specifier);

#endif