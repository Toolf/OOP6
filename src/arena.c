#include <stddef.h>
#include <stdbool.h>
#include "header.h"
#include "arena.h"
#include "kernal.h"

// створює арену
struct Arena *init_arena(void)
{
    struct Arena *default_arena = kernal_alloc(DEFAULT_ARENA_MAX_SIZE);
    if (!default_arena)
    {
        // kernal_alloc не може виділити память
        return NULL;
    }

    default_arena->body = (char *)default_arena + ARENA_HEADER_SIZE;
    default_arena->size = DEFAULT_ARENA_MAX_SIZE - ARENA_HEADER_SIZE;

    default_arena->next = NULL;
    default_arena->prev = NULL;

    // free block
    create_header(
        (struct Header *)default_arena->body, // header
        (struct Header *)NULL,                // prev
        (struct Header *)NULL,                // next
        true,                                 // free
        default_arena->size - HEADER_SIZE     // size
    );

    return default_arena;
}

void add_arena(struct Arena *arena, struct Arena *next_arena)
{
    while (arena->next)
        arena = arena->next;

    arena->next = next_arena;
    next_arena->prev = arena;
}

void remove_arena(struct Arena *arena)
{
    if (arena->prev)
        arena->prev->next = arena->next;
    if (arena->next)
        arena->next->prev = arena->prev;
    kernal_free(arena, arena->size + ARENA_HEADER_SIZE);
}

struct Arena *create_big_arena(size_t size, size_t critical_size)
{
    size = align(size);
    size_t allocate_size = size + ARENA_HEADER_SIZE + HEADER_SIZE;
    size_t big_arena_size;

    if (allocate_size % get_page_size() != 0)
        allocate_size = allocate_size + (get_page_size() - (allocate_size % get_page_size()));

    void *ptr = kernal_alloc(allocate_size);

    if (!ptr)
    {
        // kernal_alloc не може виділити память
        return NULL;
    }

    // Якщо можна виділити частину виділиної ділянки на default арену
    // if (allocate_size - size - ARENA_HEADER_SIZE >= critical_size)
    // {
    //     big_arena_size = size + ARENA_HEADER_SIZE;
    // }
    // else
    // {
    //     big_arena_size = allocate_size;
    // }
    big_arena_size = allocate_size;

    struct Arena *big_arena = ptr;

    big_arena->body = (char *)big_arena + ARENA_HEADER_SIZE;
    big_arena->size = big_arena_size;

    big_arena->next = NULL;
    big_arena->prev = NULL;

    create_header(
        (struct Header *)big_arena->body, // header
        (struct Header *)NULL,            // prev
        (struct Header *)NULL,            // next
        false,                            // free
        big_arena->size - HEADER_SIZE     // size
    );

    return big_arena;
}