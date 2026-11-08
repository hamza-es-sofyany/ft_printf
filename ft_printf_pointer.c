/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf_pointer.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hes-sofy <hes-sofy@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/05 17:18:51 by hes-sofy          #+#    #+#             */
/*   Updated: 2026/11/08 10:57:51 by hes-sofy         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

static int	ft_print_address_hex(unsigned long long n, const char *base)
{
	int		i;
	int		len;
	char	dest[17];
	int		check;

	if (n == 0)
		return (ft_print_char('0'));
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
		check = ft_print_char(dest[i]);
		if (check == -1)
			return (-1);
		len += check;
	}
	return (len);
}

int	ft_print_pointer(void *ptr)
{
	int	len;
	int	check;

	if (!ptr)
		return (ft_print_string("(nil)"));
	len = ft_print_string("0x");
	if (len == -1)
		return (-1);
	check = ft_print_address_hex((unsigned long long)ptr, "0123456789abcdef");
	if (check == -1)
		return (-1);
	return (len + check);
}
