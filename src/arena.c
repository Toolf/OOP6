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

    default_arena->size = DEFAULT_ARENA_MAX_SIZE;

    return default_arena;
}

void remove_arena(struct Arena *arena)
{
    kernal_free(arena, arena->size + ARENA_HEADER_SIZE);
}

struct Arena *create_big_arena(size_t size, size_t critical_size)
{
    size_t allocate_size = size + ARENA_HEADER_SIZE + HEADER_SIZE;
    size_t big_arena_size;

    allocate_size = align_by(allocate_size, get_page_size());

    // Перевірка переповнення
    if (allocate_size < size)
        return NULL;

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
    big_arena_size = allocate_size - ARENA_HEADER_SIZE;

    struct Arena *big_arena = ptr;

    big_arena->size = big_arena_size;

    return big_arena;
}

void decommit(struct Arena *arena, size_t addr, size_t size)
{
    kernal_decommit((char *)arena + addr, size);
}

void commit(struct Arena *arena, size_t addr, size_t size)
{
    kernal_commit((char *)arena + addr, size);
}