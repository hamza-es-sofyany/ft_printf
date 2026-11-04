/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf_hex.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hes-sofy <hes-sofy@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/04 16:18:14 by hes-sofy          #+#    #+#             */
/*   Updated: 2026/11/04 17:00:06 by hes-sofy         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

static int	ft_print_hex_digits(unsigned int n, const char *base)
{
	int		i;
	int		len;
	char	dest[15];

	if (n == 0)
	{
		dest[0] = '0';
		ft_write_char(dest[0]);
		return (1);
	}
	i = 0;
	len = 0;
	while (n > 0)
	{
		dest[i] = base[n % 16];
		n /= 16;
		i++;
	}
	while (i > 0)
	{
		i--;
		len += ft_write_char(dest[i]);
	}
	return (len);
}

int	ft_print_hex(unsigned int n, char specifier)
{
	int	len;

	len = 0;
	if (specifier == 'x')
		len += ft_print_hex_digits(n, "0123456789abcdef");
	else if (specifier == 'X')
	{
		len += ft_print_hex_digits(n, "0123456789ABCDEF");
	}
	return (len);
}
