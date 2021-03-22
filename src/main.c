#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "allocator.h"
#include "config.h"

#define N 100

#define min(a, b) min_size_t(a, b)

static size_t min_size_t(int a, int b)
{
    return a < b ? a : b;
}

struct Result
{
    void *curr;
    void *prev;
    size_t curr_size;
    size_t prev_size;
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
        sum += *((char *)ptr + i);
    return sum;
}

void auto_test()
{
    // Автоматичне тестування має виявити помилки в реалізації, які не були виявлені при ручному тестуванні.
    // Ідея тестування наступна: виконати запити mem_alloc, mem_free і mem_realloc у випадковому порядку.

    // Щоб це організувати треба створити масив в якому будуть запам’ятовуватися
    // результати успішних викликів mem_alloc і mem_realloc. Ці результати це
    // покажчики на блоки, їх розмір і контрольні суми даних блоків.
    // Всі отримані блоки необхідно заповнювати випадковими даними і підраховувати
    // їх контрольні суми. Перед викликом mem_realloc або mem_free необхідно перевірити
    // контрольну суму блоку. Контрольна сума дозволяє виявити модифікацію даних блоку.
    // Після закінчення тестування треба перевірити всі контрольні суми та звільнити всі блоки.
    unsigned int seed = time(NULL);
    srand(seed);
    printf("seed: %u\n", seed);
    struct Result results[N + 1];
    unsigned int results_index = 0;
    printf("TEST START\n");

    for (unsigned int i = 0; i < N; i++)
    {
        // 0 - ALLOC
        // 1 - REALLOC
        // 2 - FREE
        unsigned short action = rand() % 3;
        size_t size = rand();
        unsigned int rand_index = rand() % (results_index || 1);
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
                    .prev = NULL,
                    .curr_size = size,
                    .prev_size = 0,
                    .checksum = get_checksum(ptr, size),
                };
                results[results_index] = result;
                results_index++;
            }
            break;
        case 1:
            if (results_index == 0)
                break;
            // вибераємо рандомний алоцируваний блок
            result = results[rand_index];
            assert(get_checksum(result.curr, result.curr_size) == result.checksum && "bad checksum");
            unsigned int controll = get_checksum(result.curr, min(size, result.curr_size));
            void *ptr1 = mem_realloc(result.curr, size);
            if (ptr1)
            {
                assert(get_checksum(ptr1, min(size, result.curr_size)) == controll && "bad checksum");
                randomize_place(ptr1, size);
                results[rand_index] = (struct Result){
                    .curr = ptr1,
                    .prev = results[rand_index].curr,
                    .curr_size = size,
                    .prev_size = results[rand_index].curr_size,
                    .checksum = get_checksum(ptr1, size),
                };
            }
            break;
        case 2:
            if (results_index == 0)
                break;
            result = results[rand_index];
            assert(get_checksum(result.curr, result.curr_size) == result.checksum && "bad checksum");
            mem_free(result.curr);
            for (int i = rand_index; i < results_index - 1; i++)
                results[i] = results[i + 1];
            results_index--;
            break;
        default:
            break;
        }
    }

    // clean all alloc blocks

    for (unsigned int i = 0; i < results_index; i++)
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
    mem_free(ptr1);

    void *ptr3 = mem_alloc(SIZE_MAX - ALIGNMENT);
    if (!ptr3)
        printf("Test correct for mem_realloc(SIZE_MAX - ALIGNMENT)\n");
}

int main()
{
    auto_test();
    test_size_max();
    return 0;
}