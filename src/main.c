#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allocator.h"
#include "header.h"

#include "rb_tree.h"
int main()
{
    char *str1 = mem_alloc(16);
    if (!str1)
    {
        printf("str1.1 bag");
        return 0;
    }
    strcpy(str1, "1111111111111111");
    printf("str1.1: %s %p\n", str1, str1);
    char *str2 = mem_alloc(16);
    if (!str2)
    {
        printf("str2 bag");
        return 0;
    }
    strcpy(str2, "2222222222222222");
    str1 = mem_realloc(str1, 32);
    if (!str1)
    {
        printf("str1.2 bag");
        return 0;
    }
    printf("str1.2: %s %p\n", str1, str1);
    char *str3 = mem_alloc(16);
    str3 = mem_realloc(str3, 32);
    if (!str3)
    {
        printf("str3 bag");
        return 0;
    }
    printf("str1: %s %p\n", str1, str1);
    printf("str2: %s %p\n", str2, str2);
    printf("str3: %s %p\n", str3, str3);

    // free
    mem_free(str1);
    mem_free(str2);
    mem_free(str3);

    return 0;
}