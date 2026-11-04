/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf_char.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hes-sofy <hes-sofy@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/04 15:21:26 by hes-sofy          #+#    #+#             */
/*   Updated: 2026/11/04 15:39:52 by hes-sofy         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

int	ft_print_char(int c)
{
	unsigned char	ch;

	ch = (unsigned char)c;
	write(1, &ch, 1);
	return (1);
}

int	ft_print_string(const char *s)
{
	size_t	i;
	size_t	len;

	len = 0;
	i = 0;
	if (!s)
	{
		len = ft_write_string("(null)");
		return (len);
	}
	while (s[i])
	{
		len += ft_print_char((int)s[i]);
		i++;
	}
	return (len);
}
