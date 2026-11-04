/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf_number.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hes-sofy <hes-sofy@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/04 15:40:40 by hes-sofy          #+#    #+#             */
/*   Updated: 2026/11/04 16:16:44 by hes-sofy         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

static int	ft_print_digits(long nbr)
{
	char	str[11];
	int		i;
	int		len;

	len = 0;
	i = 0;
	while (nbr > 0)
	{
		str[i] = nbr % 10 + 48;
		nbr /= 10;
		i++;
	}
	while (i > 0)
	{
		i--;
		len += ft_write_char(str[i]);
	}
	return (len);
}

int	ft_print_signed(int n)
{
	long	nbr;
	int		len;

	len = 0;
	nbr = n;
	if (nbr == 0)
	{
		len += ft_write_char('0');
		return (len);
	}
	if (nbr < 0)
	{
		len += ft_write_char('-');
		nbr *= -1;
	}
	len += ft_print_digits(nbr);
	return (len);
}

int	ft_print_unsigned(unsigned int n)
{
	int	len;

	if (n == 0)
	{
		len = ft_write_char('0');
		return (len);
	}
	len = ft_print_digits((long)n);
	return (len);
}
