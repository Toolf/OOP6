#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdbool.h"
#include "allocator.h"

int test1()
{
    void *ptr, *ptr1;
    ptr = mem_alloc(0);
    mem_print();
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

bool test3()
{
    char *test_arr[10];
    for (int i = 0; i < 10; i++)
    {
        test_arr[i] = mem_alloc(128);

        if (!test_arr[i])
            return false;

        strcpy(test_arr[i], "Hello, world!");
    }
    for (int i = 0; i < 10; i++)
    {
        printf("#%d: {addr: %p, value: %s}\n", i + 1, test_arr[i], test_arr[i]);
        mem_free(test_arr[i]);
    }

    return true;
}

bool test4()
{
    size_t mem_size[9] = {10, 15, 68, 2, 98, 77, 1024, 33, 69};
    char *ptrs[9];
    for (int i = 0; i < 9; i++)
    {
        ptrs[i] = mem_alloc(mem_size[i]);
    }
    mem_print();
    for (int i = 0; i < 9; i += 2)
    {
        mem_free(ptrs[i]);
        ptrs[i] = NULL;
    }
    mem_print();
    for (int i = 1; i < 9; i += 2)
    {
        mem_free(ptrs[i]);
    }
    return true;
}

int main()
{
    printf("\nTest 1\n\n");
    test1();

    printf("\nTest 2\n\n");
    test2();

    printf("\nTest 3\n\n");
    test3();

    printf("\nTest 4\n\n");
    test4();
    return 0;
}