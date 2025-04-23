#include "myMalloc.h"
#include <sys/resource.h>
#include <stdio.h>

int main(void)
{
    struct rlimit rlim;

    if (getrlimit(RLIMIT_DATA, &rlim) == 0)
    {
        printf("Current Resource Limit (RLIMIT_AS):\n");
    }
    else
    {
        perror("getrlimit failed");
    }
    return 0;
}
