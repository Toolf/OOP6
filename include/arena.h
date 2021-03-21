#pragma once

#include "config.h"
#include <header.h>

#include "kernal.h"

// page count
#define ARENA_HEADER_SIZE align(sizeof(struct Arena))
#define DEFAULT_ARENA_MAX_SIZE ((2 * get_page_size()) - ARENA_HEADER_SIZE)

struct Arena
{
    size_t size; // розмір body
};
// створює арену
struct Arena *init_arena();

// добавляє арену в ланцюг арен (в самий кінець)
void add_arena(struct Arena *arena, struct Arena *next_arena);

void remove_arena(struct Arena *arena);

struct Arena *create_big_arena(size_t size, size_t critical_size);