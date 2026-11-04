/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf_utils.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hes-sofy <hes-sofy@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/04 13:30:20 by hes-sofy          #+#    #+#             */
/*   Updated: 2026/11/04 15:19:23 by hes-sofy         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

int	ft_write_char(char c)
{
	write(1, &c, 1);
	return (1);
}

int	ft_write_string(const char *s)
{
	int	i;

	i = 0;
	while (s[i])
	{
		ft_write_char(s[i]);
		i++;
	}
	return (i);
}

// static int	ft_base_is_tru(const char *base)
// {
// 	int	i;
// 	int	j;

// 	i = 0;
// 	while (base[i])
// 	{
// 		j = 1;
// 		while (base[i + j])
// 		{
// 			if (base[i] == base[i + j])
// 				return (0);
// 			j++;
// 		}
// 		if ((base[i] == '-' || base[i] == '+') || base[i] == ' '
// 			|| (base[i] >= 9 && base[i] <= 13))
// 			return (0);
// 		i++;
// 	}
// 	if (i < 2)
// 		return (0);
// 	return (1);
// }

// static void	ft_print(unsigned long long n, const char *base2, int *len)
// {
// 	int		i;
// 	int		to_base;
// 	char	str[64];

// 	to_base = ft_strlen(base2);
// 	i = 0;
// 	if (n == 0)
// 		str[i++] = base2[0];
// 	while (n > 0)
// 	{
// 		str[i] = base2[n % to_base];
// 		n = n / to_base;
// 		i++;
// 	}
// 	while (i > 0)
// 	{
// 		ft_write_char(str[--i]);
// 		++*len;
// 	}
// }

// int	ft_putnbr_base(unsigned long long nbr, const char *base)
// {
// 	int	i;
// 	int	len;

// 	len = 0;
// 	i = ft_base_is_tru(base);
// 	if (i == 0)
// 		return (0);
// 	ft_print(nbr, base, &len);
// 	return (len);
// }
