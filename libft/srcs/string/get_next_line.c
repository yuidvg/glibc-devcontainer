/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ynishimu <ynishimu@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/02/06 14:34:30 by ynishimu          #+#    #+#             */
/*   Updated: 2023/04/12 18:11:58 by ynishimu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/libft.h"

#ifndef BUFFER_SIZE
# define BUFFER_SIZE 32
#endif

static char	*ft_strappend(char *save, const char *const buf)
{
	char	*apnd;
	size_t	i;
	size_t	save_len;
	size_t	buf_len;

	save_len = ft_strlen(save);
	buf_len = ft_strlen(buf);
	apnd = (char *)malloc(sizeof(char) * (save_len + buf_len + 1));
	if (!apnd)
		return (NULL);
	i = 0;
	while (i < save_len)
	{
		apnd[i] = save[i];
		i++;
	}
	while (i < (save_len + buf_len))
	{
		apnd[i] = buf[i - save_len];
		i++;
	}
	apnd[i] = '\0';
	free(save);
	return (apnd);
}

static size_t	ft_strlen_c(const char *const str, int c)
{
	size_t	i;
	size_t	len;

	if (!str)
		return (0);
	i = 0;
	len = ft_strlen(str);
	while (i < len)
	{
		if (str[i] == (char)c)
			return (i + 1);
		i++;
	}
	return (0);
}

static char	*get_line(const char *const save)
{
	char	*line;
	size_t	i;
	size_t	line_len;

	i = 0;
	line_len = ft_strlen_c(save, '\n');
	if (line_len == 0)
		line_len = ft_strlen(save);
	if (line_len == 0)
		return (NULL);
	line = (char *)malloc(sizeof(char) * (line_len + 1));
	if (!line)
		return (NULL);
	i = 0;
	while (i < line_len)
	{
		line[i] = save[i];
		i++;
	}
	line[i] = '\0';
	return (line);
}

static char	*get_save(char *old_save)
{
	char	*new_save;
	size_t	i;
	size_t	j;

	i = 0;
	if (!old_save)
		return (NULL);
	while (old_save[i] != '\n' && old_save[i] != '\0')
		i++;
	if (old_save[i] == '\0')
	{
		free(old_save);
		return (NULL);
	}
	new_save = (char *)malloc((ft_strlen(old_save) - i + 1));
	if (!new_save)
		return (NULL);
	i++;
	j = 0;
	while (old_save[i])
		new_save[j++] = old_save[i++];
	new_save[j] = '\0';
	free(old_save);
	return (new_save);
}

char	*get_next_line(int fd)
{
	static char	*saves[OPEN_MAX + 1];
	char		*ret;
	char		buf[BUFFER_SIZE + 1];
	ssize_t		read_size;

	if (fd < 0 || BUFFER_SIZE <= 0)
		return (NULL);
	read_size = 1;
	while (read_size != 0 && ft_strlen_c(saves[fd], '\n') == 0)
	{
		read_size = read(fd, buf, BUFFER_SIZE);
		if (read_size < 0)
		{
			free(saves[fd]);
			return (NULL);
		}
		buf[read_size] = '\0';
		saves[fd] = ft_strappend(saves[fd], buf);
	}
	ret = get_line(saves[fd]);
	saves[fd] = get_save(saves[fd]);
	return (ret);
}
