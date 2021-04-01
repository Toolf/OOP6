#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "allocator.h"
#include "config.h"

#define N 100000
#define MAX_ARRAY_SIZE 500

#define min(a, b) min_size_t(a, b)

static size_t min_size_t(size_t a, size_t b)
{
    return a < b ? a : b;
}

struct Result
{
    void *curr;
    size_t curr_size;
    unsigned int checksum;
};

void randomize_place(void *ptr, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        *((char *)ptr + i) = (char)rand();
    }
}

unsigned int get_checksum(void *ptr, size_t size)
{
    unsigned int sum = 0;
    for (size_t i = 0; i < size; i++)
        sum = (sum << 2) ^ (sum >> 5) ^ *((char *)ptr + i);
    return sum;
}

void auto_test(size_t max_size)
{
    unsigned int seed = time(NULL);
    srand(seed);
    struct Result results[MAX_ARRAY_SIZE] = {{.checksum = 0, .curr = NULL, .curr_size = 0}};
    printf("TEST START\nseed: %u\n", seed);

    for (unsigned int i = 0; i < N; i++)
    {
        // 0 - ALLOC
        // 1 - REALLOC
        // 2 - FREE
        // if (i % (N / 100) == 0)
        //     printf("%u\n", i / (N / 100));
        unsigned short action = rand() % 3;
        size_t size = rand() % max_size;
        unsigned int rand_index = rand() % MAX_ARRAY_SIZE;
        struct Result result;
        void *ptr;
        switch (action)
        {
        case 0: // ALLOC
            ptr = mem_alloc(size);
            if (ptr)
            {
                randomize_place(ptr, size);
                result = (struct Result){
                    .curr = ptr,
                    .curr_size = size,
                    .checksum = get_checksum(ptr, size),
                };
                if (results[rand_index].curr)
                    mem_free(results[rand_index].curr);
                results[rand_index] = result;
            }
            break;
        case 1:
            result = results[rand_index];
            if (result.curr) // Якщо був алоцирований
                assert(get_checksum(result.curr, result.curr_size) == result.checksum && "bad checksum");
            unsigned int controll = get_checksum(result.curr, min(size, result.curr_size));
            void *ptr1 = mem_realloc(result.curr, size);
            if (ptr1)
            {
                assert(get_checksum(ptr1, min(size, result.curr_size)) == controll && "bad checksum");
                randomize_place(ptr1, size);
                results[rand_index] = (struct Result){
                    .curr = ptr1,
                    .curr_size = size,
                    .checksum = get_checksum(ptr1, size),
                };
            }
            break;
        case 2:
            result = results[rand_index];
            if (result.curr) // Якщо був алоцирований
                assert(get_checksum(result.curr, result.curr_size) == result.checksum && "bad checksum");
            mem_free(result.curr);
            results[rand_index] = (struct Result){
                .curr = NULL,
                .curr_size = 0,
                .checksum = 0,
            };

            break;
        default:
            break;
        }
    }

    // clean all alloc blocks

    for (unsigned int i = 0; i < MAX_ARRAY_SIZE; i++)
    {
        assert(get_checksum(results[i].curr, results[i].curr_size) == results[i].checksum && "bad checksum");
        mem_free(results[i].curr);
    }

    printf("TEST END\n");
}

void test_size_max()
{
    void *ptr = mem_alloc(SIZE_MAX);
    if (!ptr)
        printf("Test correct for mem_alloc(SIZE_MAX)\n");

    void *ptr1 = mem_alloc(16);
    if (!ptr1)
        return;

    void *ptr2 = mem_realloc(ptr1, SIZE_MAX);
    if (!ptr2)
        printf("Test correct for mem_realloc(SIZE_MAX)\n");
    ptr2 = mem_realloc(ptr1, SIZE_MAX - ALIGNMENT);
    if (!ptr2)
        printf("Test correct for mem_realloc(SIZE_MAX - ALIGNMENT)\n");
    mem_free(ptr1);

    void *ptr3 = mem_alloc(SIZE_MAX - ALIGNMENT);
    if (!ptr3)
        printf("Test correct for mem_alloc(SIZE_MAX - ALIGNMENT)\n");
}

int main()
{
    // test with very small allocation max_size
    printf("test with very small allocation max_size\n");
    auto_test(16);
    // test with small allocation max_size
    printf("test with small allocation max_size\n");
    auto_test(512);
    // test with big allocation max_size
    printf("test with big allocation max_size\n");
    auto_test(RAND_MAX);
    test_size_max();
    return 0;
}