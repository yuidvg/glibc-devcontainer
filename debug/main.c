#include "debug.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libft.h"

static ssize_t ft_putsize_t_fd(size_t n, char *digits, int fd)
{
    ssize_t printed;

    printed = 0;
    if (n >= 10)
        printed += ft_putsize_t_fd(n / 10, digits, fd);
    printed += ft_putchar_fd(digits[n % 10], fd);
    return (printed);
}

int main()
{
    ft_putsize_t_fd(32, "0123456789", 1);

    return 0;
}
