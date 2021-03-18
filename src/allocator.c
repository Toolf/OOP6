#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "allocator.h"
#include "arena.h"
#include "rb_tree.h"

#define ALIGNMENT 8 /*in bytes*/
#define HEADER_SIZE sizeof(struct Header)
// CRITICAL_SIZE - розмір мінімального порожнього блоку (включно з Header)
// TODO : Tree в майбутньому потрібно добавити NODE_SIZE
#define CRITICAL_SIZE (1 + HEADER_SIZE + NODE_SIZE)

#define min(a, b) (((a) < (b)) ? (a) : (b))

struct Arena *default_arena = NULL;

void *
mem_alloc(size_t size)
{
    // TODO : Allocator Якщо розмір який хоче виділити користувач більший за DEFAULT_ARENA_MAX_SIZE
    //        тоді потрібно виділити нову арену під розмір користувача

    // Створення початкової арени та першого блоку
    if (!default_arena)
    {
        // Примітка:
        //  Розмір default арени має бути більший ніж CRITICAL_SIZE
        default_arena = init_arena();
        if (!default_arena)
            return NULL;

        // free block
        create_header(
            (struct Header *)default_arena->body, // header
            (struct Header *)NULL,                // prev
            (struct Header *)NULL,                // next
            true,                                 // free
            default_arena->size - HEADER_SIZE     // size
        );
        default_arena->first_block = (void *)default_arena->body;

        // init root of tree
        // TODO : Tree Створення початкової ноди дерева
        insert_item(
            &default_arena->tree,
            init_node((char *)(void *)(default_arena->first_block) + HEADER_SIZE, default_arena->first_block->size) //
        );
    }
    // Вирівнювання
    if (size % ALIGNMENT != 0)
    {
        size = size + (ALIGNMENT - (size % ALIGNMENT));
        //            ^Скільки потрібно додати до розміру, щоб вирівняти по ALIGNMENT
    }

    // Пошук вільго блоку в арені потрібного розміру
    // TODO : Allocator на даний момент пошук лінійний O(N), після реалізації дерева потрібно переробити на log(N)
    // struct Header *block = default_arena->first_block;
    // while (block && !(block->free && block->size >= size))
    // {
    //     block = (struct Header *)block->next;
    // }
    struct Header *block = (void *)((char *)(void *)searchSmallestLargets(&default_arena->tree, size) - HEADER_SIZE);

    // struct Header *block = search(default_arena->tree, size);

    // Якщо знайдений block рівний NULL це значить що в даній арені не було знайдено блоку потрібного розміру
    if (block == NULL)
    {
        // TODO : Arena створення нової default арени
        return NULL;
    }

    // block був найдений
    // Розбиваємо даний блок на два дочірні
    // перший має розмір який потрібен користувачу
    // другий має розмір який залишився
    // Примітка:
    //  Якщо розмір другого блоку менший певної межі (?CRITICAL_SIZE?)
    //  block не розбивається на 2 частини, а повертається як є

    block->free = false;
    // TODO : Tree Видалення занятої ноди з дерева
    remove_item(&default_arena->tree, (void *)((char *)(void *)block + HEADER_SIZE));
    if (block->size - size > CRITICAL_SIZE)
    {
        // створення правого блоку
        struct Header *right_sub_block = (void *)((char *)(void *)block + HEADER_SIZE + size);
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

        // TODO : Tree Добавлення нової ноди в дерево
        insert_item(
            &default_arena->tree,
            init_node((char *)(void *)right_sub_block + HEADER_SIZE, right_sub_block->size) //
        );
    }

    return (char *)block + HEADER_SIZE;
}

void mem_free(void *ptr)
{
    struct Header *header = (void *)((char *)ptr - HEADER_SIZE);
    header->free = true;

    // TODO : Allocator Пошук арени якій належить header

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

        // TODO : Tree Добавлення нової ноди в дерево та видалення двох інших
        remove_item(&default_arena->tree, (void *)((char *)(void *)header->prev + HEADER_SIZE));
        remove_item(&default_arena->tree, (void *)((char *)(void *)header->next + HEADER_SIZE));
        insert_item(&default_arena->tree, init_node((char *)(void *)header->prev + HEADER_SIZE, header->prev->size));
    }
    else if (header->next && header->next->free)
    {
        header->size = header->size + header->next->size + HEADER_SIZE;
        header->next = header->next->next;
        if (header->next)
            header->next->prev = header;

        // TODO : Tree Добавлення нової ноди в дерево та видалення ноди наступного блоку однії
        remove_item(&default_arena->tree, (void *)((char *)(void *)header->next + HEADER_SIZE));
        insert_item(&default_arena->tree, init_node((char *)(void *)header->prev + HEADER_SIZE, header->size));
    }
    else if (header->prev && header->prev->free)
    {
        header->prev->size = header->prev->size + header->size + HEADER_SIZE;
        header->prev->next = header->next;
        if (header->prev->next)
            header->prev->next->prev = header->prev;

        // TODO : Tree Одновлення ноди в дереві
        remove_item(&default_arena->tree, (void *)((char *)(void *)header->prev + HEADER_SIZE));
        insert_item(&default_arena->tree, init_node((char *)(void *)header->prev + HEADER_SIZE, header->prev->size));
    }
    else
    {
        insert_item(&default_arena->tree, init_node((char *)(void *)header + HEADER_SIZE, header->size));
    }

    /* TODO: 
     * Якщо якась сторінка пам’яті в алокаторі пам’яті не містить жодної інформації, 
     * то алокатор має повідомити ядро про це відповідним системним викликом.
     */
    // Якщо даний блок єдиний на всю арену
    // то потрібно повідомити ядро про це відповідним системним викликом.
    // if (!header->next && !header->prev)
    // {
    //     struct Arena *arena = (void *)((char *)(void *)header - HEADER_SIZE);
    //     remove_arena(arena);
    // }
}

void *mem_realloc(void *ptr, size_t new_size)
{
    if (!ptr)
        return mem_alloc(new_size);

    // Вирівнювання
    if (new_size % ALIGNMENT != 0)
    {
        new_size = new_size + (ALIGNMENT - (new_size % ALIGNMENT));
        //                    ^Скільки потрібно додати до розміру, щоб вирівняти по ALIGNMENT
    }

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
        }
        return ptr;
    }

    // Якщо можемо розмістити в тій ж пам'яті, але потрібно склеїти з наступним блоком даний блок
    if (block->next && block->next->free && block_size + block->next->size >= new_size)
    {
        size_t merge_size = block_size + block->next->size;
        if (merge_size - new_size >= CRITICAL_SIZE)
        {
            block->size = new_size;
            struct Header *new_block = (void *)(((char *)(void *)block + HEADER_SIZE) + new_size);
            create_header(new_block, block, block->next->next, true, merge_size - new_size);
            block->next = new_block;

            // TODO : Tree Добавлення нової ноди в дерево та видалення сторої
            remove_item(&default_arena->tree, (void *)(((char *)(void *)(block->next)) + HEADER_SIZE));
            insert_item(&default_arena->tree, init_node((char *)(void *)block + HEADER_SIZE, block->size));
        }
        else
        {
            block->size = block->size + block->next->size;
            block->next = block->next->next;
            block->next->prev = block;

            // TODO : Tree Видалення сторої ноди
            remove_item(&default_arena->tree, (void *)((char *)(void *)block->next + HEADER_SIZE));
        }
        return ptr;
    }

    // Якщо cклеювання або розміщення на цьому ж місці не можливе
    void *res = mem_alloc(new_size);
    memcpy(res, ptr, min(new_size, block_size));
    mem_free(ptr);

    return res;
}