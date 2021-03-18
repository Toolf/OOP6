#pragma once

#include <header.h>
#include <rb_tree.h>

struct Arena
{
    char *body;
    unsigned int size; // розмір body
    struct Header *first_block;
    struct RBTree tree;
    // Покащо не використовується
    struct Arena *next;
    struct Arena *prev;
};

// створює арену
struct Arena *init_arena();

void add_arena(struct Arena *arena, struct Arena *next_arena);

void remove_arena(struct Arena *arena);