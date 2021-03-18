#include <stddef.h>
#include <stdbool.h>
#include "header.h"
#include "arena.h"
#include "kernal.h"

#define DEFAULT_ARENA_MAX_SIZE (512 * 8)

// static char _default_arena[DEFAULT_ARENA_MAX_SIZE];

// створює арену
struct Arena *init_arena(void)
{
    // struct Arena *default_arena = (struct Arena *)(void *)_default_arena;
    struct Arena *default_arena = kernal_alloc(DEFAULT_ARENA_MAX_SIZE);
    if (!default_arena)
    {
        // kernal_alloc не може виділити память
        return NULL;
    }

    default_arena->body = (char *)(void *)default_arena + sizeof(struct Arena);
    default_arena->size = DEFAULT_ARENA_MAX_SIZE - sizeof(struct Arena);
    default_arena->first_block = NULL;
    default_arena->tree = (struct RBTree){.root = NULL};

    default_arena->next = NULL;
    default_arena->prev = NULL;

    return default_arena;
}

void add_arena(struct Arena *arena, struct Arena *next_arena)
{
    // TODO : Arena Добавлення нової арени
    arena->next = next_arena;
    next_arena->prev = arena;
}

void remove_arena(struct Arena *arena)
{
    // TODO : Arena Видалення арени
    if (arena->prev)
        arena->prev->next = arena->next;
    if (arena->next)
        arena->next->prev = arena->prev;
    kernal_free(arena, arena->size + sizeof(struct Arena));
}