#pragma once

#include "config.h"
#include <header.h>
#include <rb_tree.h>

#define DEFAULT_ARENA_MAX_SIZE (512 * ALIGNMENT)
#define ARENA_HEADER_SIZE align(sizeof(struct Arena))

struct Arena
{
    char *body;
    size_t size; // розмір body
    struct Header *first_block;
    struct Arena *next;
    struct Arena *prev;
};

// створює арену
struct Arena *init_arena();

// добавляє арену в ланцюг арен (в самий кінець)
void add_arena(struct Arena *arena, struct Arena *next_arena);

void remove_arena(struct Arena *arena);

struct Arena *create_big_arena(size_t size, size_t critical_size);