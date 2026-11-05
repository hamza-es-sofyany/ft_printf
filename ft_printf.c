/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hes-sofy <hes-sofy@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/05 17:18:55 by hes-sofy          #+#    #+#             */
/*   Updated: 2026/11/05 17:55:52 by hes-sofy         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"
#include <stdarg.h>
#include <stdio.h>

static int	ft_eval(va_list list, char spec)
{
	int	check;

	check = 0;
	if (spec == 'c')
		check = ft_print_char(va_arg(list, int));
	else if (spec == 's')
		check = ft_print_string(va_arg(list, char *));
	else if (spec == 'p')
		check = ft_print_pointer(va_arg(list, void *));
	else if (spec == 'd' || spec == 'i')
		check = ft_print_signed(va_arg(list, int));
	else if (spec == 'u')
		check = ft_print_unsigned(va_arg(list, unsigned int));
	else if (spec == 'x' || spec == 'X')
		check = ft_print_hex(va_arg(list, unsigned int), spec);
	else if (spec == '%')
		check = ft_print_char('%');
	return (check);
}

int	ft_printf(const char *format, ...)
{
	va_list	list;
	int		len;
	int		i;
	int		check;

	va_start(list, format);
	i = 0;
	len = 0;
	while (format[i])
	{
		if (format[i] == '%' && format[i + 1])
		{
			check = ft_eval(list, format[i + 1]);
			if (check == -1)
				return (-1);
			len += check;
			i++;
		}
		else
		{
			check = ft_print_char(format[i]);
			if (check == -1)
				return (-1);
			len += check;
		}
		i++;
	}
	va_end(list);
	return (len);
}
