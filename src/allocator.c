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

static struct RBTree global_tree = {.root = NULL};

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

#define block_to_node(block) (struct Node *)((char *)block + HEADER_SIZE)
#define node_to_block(node) (struct Header *)((char *)node - HEADER_SIZE)
#define block_to_payload(block) ((char *)block + HEADER_SIZE)
#define payload_to_block(payload) (struct Header *)((char *)payload - HEADER_SIZE)
#define arena_to_first_block(arena) (struct Header *)((char *)arena + ARENA_HEADER_SIZE)
#define block_to_arena(block) (struct Arena *)((char *)block - block_get_addr(block))

void block_decommit(struct Header *block)
{
    size_t addr_start;
    addr_start = align_by(block_get_addr(block), get_page_size());
    if (addr_start < block_get_addr(block) + HEADER_SIZE + NODE_SIZE)
        addr_start += get_page_size();
    if (addr_start + get_page_size() <= block_get_addr(block) + block->size)
    {
        // якщо початок та кінець області для decommit знаходиться в блоку
        size_t decommit_size = HEADER_SIZE + block->size - addr_start;
        size_t align_to_lower_decommit_size =
            decommit_size % get_page_size() == 0 ? decommit_size
                                                 : decommit_size - decommit_size % get_page_size();
        struct Arena *arena = block_to_arena(block);
        decommit(arena, addr_start, align_to_lower_decommit_size);
    }
}

void block_commit(struct Header *block)
{
    size_t addr_start;
    addr_start = align_by(block_get_addr(block), get_page_size());
    size_t decommit_size = HEADER_SIZE + block->size - addr_start;
    size_t align_to_lower_decommit_size =
        decommit_size % get_page_size() == 0 ? decommit_size
                                             : decommit_size - decommit_size % get_page_size();
    struct Arena *arena = block_to_arena(block);
    commit(arena, addr_start, align_to_lower_decommit_size);
}

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
        create_header(block, NULL, false, big_arena->size - HEADER_SIZE, ARENA_HEADER_SIZE);
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
        create_header(block, NULL, true, new_arena->size - HEADER_SIZE, ARENA_HEADER_SIZE);
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
    block_commit(block);
    remove_item(&global_tree, block_to_node(block));
    size_t block_size = block_get_size_curr(block);

    if (block_size - size >= CRITICAL_SIZE)
    {
        // створення правого блоку
        bool is_last = block_is_last(block);
        if (is_last)
        {
            block_unset_last(block);
        }

        block_set_size_curr(block, size);
        struct Header *right_sub_block = block_next(block);
        create_header(
            right_sub_block,                                   /*Початок блоку*/
            block,                                             /*Попередній блок*/
            true,                                              /*Стан блоку "вільний"*/
            block_size - size - HEADER_SIZE,                   /*Розмір блоку*/
            block_get_addr(block) + block_get_size_curr(block) //
        );
        if (!is_last)
        {
            block_unset_last(right_sub_block);
            block_set_size_prev(block_next(right_sub_block), block_get_size_curr(right_sub_block));
        }

        // Добавлення нової ноди в дерево
        block_decommit(right_sub_block);
        insert_item(
            &global_tree,
            init_node(block_to_node(right_sub_block), block_get_size_curr(right_sub_block)) //
        );
    }

    return block_to_payload(block);
}

void mem_free(void *ptr)
{
    if (!ptr)
        return;

    struct Header *block = payload_to_block(ptr);

    if (block_is_first(block) && ((struct Arena *)block_to_arena(block))->size > DEFAULT_ARENA_MAX_SIZE)
    {
        // big arena
        struct Arena *arena = block_to_arena(block);
        remove_arena(arena);
        return;
    }

    block_set_free(block);
    insert_item(&global_tree, init_node(block_to_node(block), block_get_size_curr(block)));

    // Склеювання з сусудніми блоками, якщо вони також вільні.
    block = block_merge(block, block_next(block));
    block = block_merge(block_prev(block), block);

    // Якщо даний блок єдиний на всю арену
    // то потрібно повідомити ядро про це відповідним системним викликом.
    if (block_is_first(block) && block_is_last(block))
    {
        struct Arena *arena = block_to_arena(block);
        remove_item(&global_tree, block_to_node(block));
        remove_arena(arena);
    }
    else
    {
        // Якщо якась сторінка пам’яті в алокаторі пам’яті не містить жодної інформації,
        // то алокатор має повідомити ядро про це відповідним системним викликом.

        block_decommit(block);
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

    if (block_is_first(block) && ((struct Arena *)block_to_arena(block))->size > DEFAULT_ARENA_MAX_SIZE)
    {
        // Якщо big arena
        struct Arena *arena = block_to_arena(block);
        size_t new_size_align_by_page_size = align_by(new_size, get_page_size());
        if (arena->size - HEADER_SIZE == new_size_align_by_page_size)
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
                create_header(
                    new_block,
                    block,
                    true,
                    block_size - new_size - HEADER_SIZE,
                    block_get_addr(block) + block_get_size_curr(block) //
                );
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
            block_commit(next);
            size_t merge_size = block_get_size_curr(block) + block_get_size_curr(next) + HEADER_SIZE;
            if (merge_size - new_size >= CRITICAL_SIZE)
            {
                // Добавлення нової ноди в дерево та видалення сторої
                remove_item(&global_tree, block_to_node(next));
                bool is_last = block_is_last(next);
                block_set_size_curr(block, new_size);
                struct Header *new_block = block_next(block);
                create_header(
                    new_block,
                    block,
                    true,
                    merge_size - new_size - HEADER_SIZE,
                    block_get_addr(block) + block_get_size_curr(block) //
                );
                if (!is_last)
                {
                    block_unset_last(new_block);
                    block_set_size_prev(block_next(new_block), block_get_size_curr(new_block));
                }

                block_decommit(new_block);
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
