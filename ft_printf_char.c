/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf_char.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hes-sofy <hes-sofy@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/05 17:18:31 by hes-sofy          #+#    #+#             */
/*   Updated: 2026/11/08 13:15:14 by hes-sofy         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

int	ft_print_char(int c)
{
	unsigned char	ch;

	ch = (unsigned char)c;
	if (write(1, &ch, 1) != 1)
		return (-1);
	return (1);
}

int	ft_print_string(const char *s)
{
	size_t	i;
	size_t	len;
	int		check;

	len = 0;
	i = 0;
	if (!s)
		s = "(null)";
	while (s[i])
	{
		check = ft_print_char((int)s[i]);
		if (check == -1)
			return (-1);
		len += check;
		i++;
	}
	return (len);
}
