/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf_pointer.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hes-sofy <hes-sofy@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/05 17:18:51 by hes-sofy          #+#    #+#             */
/*   Updated: 2026/11/05 17:18:52 by hes-sofy         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

static int	ft_print_address_hex(unsigned long long n, const char *base)
{
	int		i;
	int		len;
	char	dest[17];

	if (n == 0)
	{
		dest[0] = '0';
		ft_print_char(dest[0]);
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
		len += ft_print_char(dest[i]);
	}
	return (len);
}

int	ft_print_pointer(void *ptr)
{
	int	len;

	if (!ptr)
	{
		len = ft_print_string("(nil)");
		return (len);
	}
	len = ft_print_string("0x");
	len += ft_print_address_hex((unsigned long long)ptr, "0123456789abcdef");
	return (len);
}
