#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "kernal.h"
#include "allocator.h"
#include "arena.h"
#include "rb_tree.h"
// CRITICAL_SIZE - розмір мінімального порожнього блоку (включно з Header)
#define CRITICAL_SIZE (HEADER_SIZE + NODE_SIZE)

static inline size_t min(size_t a, size_t b)
{
    return a < b ? a : b;
}

static inline size_t max(size_t a, size_t b)
{
    return a > b ? a : b;
}

static struct RBTree global_tree = {.root = NULL};

void *
mem_alloc(size_t size)
{

    // Вирівнювання
    size = max(align(size), NODE_SIZE);

    // Якщо розмір який хоче виділити користувач більший за DEFAULT_ARENA_MAX_SIZE
    // тоді потрібно виділити нову арену під розмір користувача
    if (DEFAULT_ARENA_MAX_SIZE - ARENA_HEADER_SIZE - HEADER_SIZE < size)
    {
        // > Вирішив не ділити дану арени
        // Причина: (Це не точно)
        //  Не факт що я зможу в майбутньому віддати системі (windows)
        //  підчастину (default arena) яка є частиною блоку який виділила система
        struct Arena *big_arena = create_big_arena(size, CRITICAL_SIZE);
        if (!big_arena)
            return NULL;

        return ((char *)body(big_arena) + HEADER_SIZE);
    }

    // Пошук вільго блоку в арені потрібного розміру
    void *ptr = search_smallest_largets(&global_tree, size);
    struct Header *block = NULL;
    if (ptr)
    {
        block = (void *)((char *)ptr - HEADER_SIZE);
    }

    // Якщо знайдений block рівний NULL це значить що в даній арені не було знайдено блоку потрібного розміру
    if (block == NULL)
    {

        // створення нової default арени
        struct Arena *new_arena = init_arena();
        if (new_arena == NULL)
        {
            return NULL;
        }
        struct Header *first_block = body(new_arena);
        insert_item(
            &global_tree,
            init_node((char *)(first_block) + HEADER_SIZE, first_block->size) //
        );

        // Блок якого повинно бути достатньо
        block = first_block;
    }

    // block був найдений
    // Розбиваємо даний блок на два дочірні
    // перший має розмір який потрібен користувачу
    // другий має розмір який залишився
    // Примітка:
    //  Якщо розмір другого блоку менший певної межі (?CRITICAL_SIZE?)
    //  block не розбивається на 2 частини, а повертається як є

    block->free = false;
    // Видалення занятої ноди з дерева
    remove_item(&global_tree, (void *)((char *)block + HEADER_SIZE));
    if (block->size - size > CRITICAL_SIZE)
    {
        // створення правого блоку
        struct Header *right_sub_block = (void *)((char *)block + HEADER_SIZE + size);
        create_header(
            right_sub_block,                 /*Початок блоку*/
            block,                           /*Попередній блок*/
            block->next,                     /*Слідуючий блок*/
            true,                            /*Стан блоку "вільний"*/
            block->size - size - HEADER_SIZE /*Розмір блоку*/
        );
        // оновлення початкового блоку
        block->size = size;
        block->next = right_sub_block;

        // Добавлення нової ноди в дерево
        insert_item(
            &global_tree,
            init_node((char *)right_sub_block + HEADER_SIZE, right_sub_block->size) //
        );
    }

    return (char *)block + HEADER_SIZE;
}

void mem_free(void *ptr)
{
    struct Header *header = (void *)((char *)ptr - HEADER_SIZE);
    header->free = true;

    // Склеювання з сусудніми блоками, якщо вони також вільні.

    // Розглядаємо чотири варіанта
    // 1) Якщо лівий та правий блоки вільні
    // 2) Якщо лише лівий блок вільний
    // 3) Якщо лише правий блок вільний
    // 4) Якщо сусідні блоки не вільні

    if (header->next && header->prev && header->next->free && header->prev->free)
    {
        header->prev->size = header->prev->size + header->size + header->next->size + HEADER_SIZE * 2;
        header->prev->next = header->next->next;
        if (header->prev->next)
            header->prev->next->prev = header->prev;

        // Добавлення нової ноди в дерево та видалення двох інших
        remove_item(&global_tree, (void *)((char *)header->prev + HEADER_SIZE));
        remove_item(&global_tree, (void *)((char *)header->next + HEADER_SIZE));
        insert_item(&global_tree, init_node((char *)header->prev + HEADER_SIZE, header->prev->size));

        header = header->prev;
    }
    else if (header->next && header->next->free)
    {
        // Добавлення нової ноди в дерево та видалення ноди наступного блоку однії
        remove_item(&global_tree, (void *)((char *)header->next + HEADER_SIZE));

        header->size = header->size + header->next->size + HEADER_SIZE;
        header->next = header->next->next;
        if (header->next)
            header->next->prev = header;

        insert_item(&global_tree, init_node((char *)header + HEADER_SIZE, header->size));
    }
    else if (header->prev && header->prev->free)
    {
        // Одновлення ноди в дереві
        remove_item(&global_tree, (void *)((char *)header->prev + HEADER_SIZE));

        header->prev->size = header->prev->size + header->size + HEADER_SIZE;
        header->prev->next = header->next;
        if (header->prev->next)
            header->prev->next->prev = header->prev;

        insert_item(&global_tree, init_node((char *)header->prev + HEADER_SIZE, header->prev->size));

        header = header->prev;
    }
    else
    {
        insert_item(&global_tree, init_node((char *)header + HEADER_SIZE, header->size));
    }

    /* 
     * Якщо якась сторінка пам’яті в алокаторі пам’яті не містить жодної інформації, 
     * то алокатор має повідомити ядро про це відповідним системним викликом.
     */
    // Якщо даний блок єдиний на всю арену
    // то потрібно повідомити ядро про це відповідним системним викликом.
    if (!header->next && !header->prev)
    {
        struct Arena *arena = (void *)((char *)header - ARENA_HEADER_SIZE);
        remove_item(&global_tree, (void *)((char *)header + HEADER_SIZE));
        remove_arena(arena);
    }
}

void *mem_realloc(void *ptr, size_t new_size)
{
    if (!ptr)
        return mem_alloc(new_size);

    // Вирівнювання
    new_size = max(align(new_size), NODE_SIZE);

    struct Header *block = (void *)((char *)ptr - HEADER_SIZE);
    size_t block_size = block->size;

    // Якщо можемо розмістити в тій ж пам'яті без додаткових дій
    if (block_size >= new_size)
    {
        if (block_size - new_size >= CRITICAL_SIZE)
        {
            block->size = new_size;
            struct Header *new_block = (void *)((char *)ptr + new_size);
            create_header(new_block, block, block->next, true, block_size - new_size);
            block->next = new_block;

            insert_item(&global_tree, init_node((char *)new_block + HEADER_SIZE, new_block->size));
        }
        return ptr;
    }

    // Якщо можемо розмістити в тій ж пам'яті, але потрібно склеїти з наступним блоком даний блок
    if (block->next && block->next->free && block_size + block->next->size + HEADER_SIZE >= new_size)
    {
        size_t merge_size = block_size + block->next->size + HEADER_SIZE;
        if (merge_size - new_size >= CRITICAL_SIZE)
        {
            // Добавлення нової ноди в дерево та видалення сторої
            remove_item(&global_tree, (void *)(((char *)(block->next)) + HEADER_SIZE));

            block->size = new_size;
            struct Header *new_block = (void *)(((char *)block + HEADER_SIZE) + new_size);
            create_header(new_block, block, block->next->next, true, merge_size - new_size - HEADER_SIZE);
            block->next = new_block;

            insert_item(&global_tree, init_node((char *)new_block + HEADER_SIZE, new_block->size));
        }
        else
        {
            // Видалення сторої ноди
            remove_item(&global_tree, (void *)((char *)block->next + HEADER_SIZE));

            block->size = block->size + block->next->size + HEADER_SIZE;
            block->next = block->next->next;
            block->next->prev = block;
        }

        return ptr;
    }

    // Якщо cклеювання або розміщення на цьому ж місці не можливе
    void *res = mem_alloc(new_size);
    if (!res)
        return NULL;
    memcpy(res, ptr, min(new_size, block_size));
    mem_free(ptr);

    return res;
}

void mem_print(void)
{
    printf("---------------MEM START------------------\n");
    print_tree(&global_tree);
    printf("---------------MEM END--------------------\n");
}