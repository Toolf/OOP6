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

static struct RBTree global_tree = {.root = &RBNIL};

#define min(a, b) min_size_t(a, b)
#define max(a, b) max_size_t(a, b)

size_t
max_size_t(size_t a, size_t b)
{
    return a > b ? a : b;
}

size_t
min_size_t(size_t a, size_t b)
{
    return a < b ? a : b;
}

#define block_to_node(block) ((char *)block + HEADER_SIZE)
#define node_to_block(node) ((char *)node - HEADER_SIZE)
#define block_to_payload(block) ((char *)block + HEADER_SIZE)
#define payload_to_block(payload) ((char *)payload - HEADER_SIZE)
#define arena_to_first_block(arena) ((char *)arena + ARENA_HEADER_SIZE)
#define first_block_to_arena(first_block) ((char *)first_block - ARENA_HEADER_SIZE)

struct Header *block_merge(struct Header *block, struct Header *next)
{
    if (!block || !block_is_free(block))
        return next;
    if (!next || !block_is_free(next))
        return block;

    remove_item(&global_tree, block_to_node(next));
    remove_item(&global_tree, block_to_node(block));
    block_set_size_curr(block, block_get_size_curr(block) + block_get_size_curr(next) + HEADER_SIZE);
    if (!block_is_last(next))
        block_set_size_prev(block_next(block), block_get_size_curr(block));
    else
        block_set_last(block);
    insert_item(&global_tree, init_node(block_to_node(block), block_get_size_curr(block)));
    return block;
}

void *
mem_alloc(size_t size)
{
    struct Header *block = NULL;

    // Вирівнювання
    size_t _size = max(align(size), NODE_SIZE);
    // Перевірка переповнення
    if (_size < size)
        return NULL;
    size = _size;

    // Якщо розмір який хоче виділити користувач більший за DEFAULT_ARENA_MAX_SIZE
    // тоді потрібно виділити нову арену під розмір користувача
    if (DEFAULT_ARENA_MAX_SIZE - HEADER_SIZE < size)
    {
        // > Вирішив не ділити дану арени
        // Причина: (Це не точно)
        //  Не факт що я зможу в майбутньому віддати системі (windows)
        //  підчастину (default arena) яка є частиною блоку який виділила система
        struct Arena *big_arena = create_big_arena(size, CRITICAL_SIZE);
        if (!big_arena)
            return NULL;
        block = arena_to_first_block(big_arena);
        create_header(block, NULL, false, big_arena->size - HEADER_SIZE);
        block_set_first(block);
        block_set_last(block);

        return block_to_payload(block);
    }

    // Пошук вільго блоку в арені потрібного розміру
    struct Node *node = search_smallest_largets(&global_tree, size);
    if (node)
        block = node_to_block(node);

    // Якщо знайдений block рівний NULL це значить що в даній арені не було знайдено блоку потрібного розміру
    if (block == NULL)
    {

        // створення нової default арени
        struct Arena *new_arena = init_arena();
        if (new_arena == NULL)
            return NULL;

        block = arena_to_first_block(new_arena);
        create_header(block, NULL, false, new_arena->size - HEADER_SIZE);
        block_set_first(block);
        block_set_last(block);

        insert_item(
            &global_tree,
            init_node(block_to_node(block), block_get_size_curr(block)) //
        );
    }

    // block був найдений
    // Розбиваємо даний блок на два дочірні
    // перший має розмір який потрібен користувачу
    // другий має розмір який залишився
    // Примітка:
    //  Якщо розмір другого блоку менший певної межі (?CRITICAL_SIZE?)
    //  block не розбивається на 2 частини, а повертається як є

    block_unset_free(block);
    // Видалення занятої ноди з дерева
    remove_item(&global_tree, block_to_node(block));
    size_t block_size = block_get_size_curr(block);
    if (block_size - size >= CRITICAL_SIZE)
    {
        // створення правого блоку
        block_set_size_curr(block, size);
        bool is_last = block_is_last(block);
        if (is_last)
            block_unset_last(block);
        struct Header *right_sub_block = block_next(block);
        create_header(
            right_sub_block,                /*Початок блоку*/
            block,                          /*Попередній блок*/
            true,                           /*Стан блоку "вільний"*/
            block_size - size - HEADER_SIZE /*Розмір блоку*/
        );
        if (!is_last)
        {
            block_unset_last(right_sub_block);
            block_set_size_prev(block_next(right_sub_block), block_get_size_curr(right_sub_block));
        }

        // Добавлення нової ноди в дерево
        insert_item(
            &global_tree,
            init_node(block_to_node(right_sub_block), block_get_size_curr(right_sub_block)) //
        );
    }

    return block_to_payload(block);
}

void mem_free(void *ptr)
{
    struct Header *block = payload_to_block(ptr);

    if (block_is_first(block) && ((struct Arena *)first_block_to_arena(block))->size > DEFAULT_ARENA_MAX_SIZE)
    {
        // big arena
        struct Arena *arena = first_block_to_arena(block);
        remove_arena(arena);
        return;
    }

    block_set_free(block);
    insert_item(&global_tree, init_node(block_to_node(block), block_get_size_curr(block)));

    // Склеювання з сусудніми блоками, якщо вони також вільні.
    block = block_merge(block, block_next(block));
    block = block_merge(block_prev(block), block);

    // TODO : Allocator decommit
    // Якщо якась сторінка пам’яті в алокаторі пам’яті не містить жодної інформації,
    // то алокатор має повідомити ядро про це відповідним системним викликом.

    // [arena                                                                                             ]
    // [page1                         ][page2                              ][page3                        ]
    // [arena_h][[block_h]       ][block_h][[node]                                                        ]
    //                            ^block
    // ^arena                          ^arena + page_size
    //                                 ^find_first_page_start in block_h
    // find_first_page_start in block_h скорочено будем називати ffps_in_b
    // ffps_in_b = (block / page_size)(заокруглене до більшого)
    // for (ps_in_b = ffps_in_b; ps_in_b in block)

    // Якщо даний блок єдиний на всю арену
    // то потрібно повідомити ядро про це відповідним системним викликом.
    if (block_is_first(block) && block_is_last(block))
    {
        struct Arena *arena = first_block_to_arena(block);
        remove_item(&global_tree, block_to_node(block));
        remove_arena(arena);
    }
}

void *mem_realloc(void *ptr, size_t new_size)
{
    if (!ptr)
        return mem_alloc(new_size);

    if (new_size > SIZE_MAX - ARENA_HEADER_SIZE - HEADER_SIZE)
        return NULL;

    // Вирівнювання
    size_t _new_size = max(align(new_size), NODE_SIZE);
    // Перевірка переповнення
    if (_new_size < new_size)
        return NULL;
    new_size = _new_size;

    struct Header *block = payload_to_block(ptr);
    size_t block_size = block_get_size_curr(block);

    if (block_is_first(block) && ((struct Arena *)first_block_to_arena(block))->size > DEFAULT_ARENA_MAX_SIZE)
    {
        // Якщо big arena
        struct Arena *arena = first_block_to_arena(block);

        if (arena->size - HEADER_SIZE == new_size)
            return ptr;
    }
    else
    {
        // Якщо default arena

        // Якщо можемо розмістити в тій ж пам'яті без додаткових дій
        if (block_size >= new_size)
        {
            // [block_size                  ]
            // [new_size  ]   [CRITICAL_SIZE]
            // [          ][new_block       ]

            if (block_size - new_size >= CRITICAL_SIZE)
            {
                struct Header *next = block_next(block);
                block_set_size_curr(block, new_size);
                bool is_last = block_is_last(block);
                if (is_last)
                    block_unset_last(block);
                struct Header *new_block = block_next(block);
                create_header(new_block, block, true, block_size - new_size - HEADER_SIZE);
                if (!is_last)
                {
                    block_unset_last(new_block);
                    block_set_size_prev(next, block_get_size_curr(new_block));
                }

                insert_item(&global_tree, init_node(block_to_node(new_block), block_get_size_curr(new_block)));
            }
            return ptr;
        }

        // Якщо можемо розмістити в тій ж пам'яті, але потрібно склеїти з наступним блоком даний блок
        // [block           ][next              ]
        // [block                  ][new_block  ]
        struct Header *next = block_next(block);
        if (next && block_is_free(next) && block_get_size_curr(block) + block_get_size_curr(next) + HEADER_SIZE >= new_size)
        {
            size_t merge_size = block_get_size_curr(block) + block_get_size_curr(next) + HEADER_SIZE;
            if (merge_size - new_size >= CRITICAL_SIZE)
            {
                // Добавлення нової ноди в дерево та видалення сторої
                remove_item(&global_tree, block_to_node(next));
                bool is_last = block_is_last(next);
                block_set_size_curr(block, new_size);
                struct Header *new_block = block_next(block);
                create_header(new_block, block, true, merge_size - new_size - HEADER_SIZE);
                if (!is_last)
                {
                    block_unset_last(new_block);
                    block_set_size_prev(block_next(new_block), block_get_size_curr(new_block));
                }

                insert_item(&global_tree, init_node(block_to_node(new_block), block_get_size_curr(new_block)));
            }
            else
            {
                // Видалення сторої ноди
                remove_item(&global_tree, block_to_node(next));
                block_set_size_curr(block, merge_size);
                bool is_last = block_is_last(next);
                if (is_last)
                    block_set_last(block);
                else
                    block_set_size_prev(block_next(block), merge_size);
            }

            return ptr;
        }
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
