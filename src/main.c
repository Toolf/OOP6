#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allocator.h"
#include "header.h"
#include "arena.h"
#include <assert.h>

int test1()
{
    void *ptr, *ptr1;
    ptr = mem_alloc(DEFAULT_ARENA_MAX_SIZE - HEADER_SIZE - ARENA_HEADER_SIZE);
    ptr1 = mem_alloc(512);

    if (!ptr)
        return false;

    mem_print();

    ptr = mem_realloc(ptr, 512);
    mem_print();

    mem_free(ptr1);

    mem_print();

    mem_free(ptr);

    return true;
}

int test2()
{
    void *ptr = mem_alloc(1024 * 10);
    mem_print();
    void *ptr1 = mem_alloc(7096);
    mem_print();
    mem_free(ptr1);
    mem_print();
    char *str1 = mem_alloc(16);
    mem_print();
    mem_free(ptr);
    mem_print();
    if (!str1)
    {
        printf("str1.1 bag");
        return false;
    }
    strcpy(str1, "111111111111111");
    printf("str1: %s %p\n", str1, str1);
    char *str2 = mem_alloc(16);
    mem_print();
    if (!str2)
    {
        printf("str2 bag");
        return false;
    }
    strcpy(str2, "222222222222222");
    str1 = mem_realloc(str1, 32);
    mem_print();
    if (!str1)
    {
        printf("str1.2 bag");
        return false;
    }
    printf("str1.2: %s %p\n", str1, str1);
    char *str3 = mem_alloc(16);
    mem_print();
    mem_free(str3);
    mem_print();
    str3 = mem_alloc(16);
    mem_print();
    str3 = mem_realloc(str3, 32);
    mem_print();
    if (!str3)
    {
        printf("str3 bag");
        return false;
    }
    printf("str1: %s %p\n", str1, str1);
    printf("str2: %s %p\n", str2, str2);

    // free
    mem_free(str3);
    mem_print();
    mem_free(str1);
    mem_print();
    mem_free(str2);
    mem_print();
    ptr = mem_alloc(512);
    mem_print();
    mem_free(ptr);
    mem_print();
    return true;
}

int main()
{
    printf("\nTest 1\n\n");
    test1();

    printf("\nTest 2\n\n");
    test2();
    return 0;
}