#include "header.h"
#include <stdbool.h>

// Нам потрібні три біта на флаги
#define SIZE_MASK ((size_t)(-1) >> 2)
// флаги
// save in size
#define FIRST_BLOCK (size_t)((size_t)(-1) ^ ((size_t)(-1) >> 1))
#define FREE_BLOCK (size_t)(((size_t)(-1) >> 1) ^ ((size_t)(-1) >> 2))
// save in size_prev
#define LAST_BLOCK (size_t)((size_t)(-1) ^ ((size_t)(-1) >> 1))
#define DECOMMIT_BLOCK (size_t)(((size_t)(-1) >> 1) ^ ((size_t)(-1) >> 2))

size_t block_get_size_curr(struct Header *block)
{
    return (block->size & SIZE_MASK) * ALIGNMENT;
}
void block_set_size_curr(struct Header *block, size_t size)
{
    size_t new_size = (size / ALIGNMENT);
    size_t flags = block->size & (~SIZE_MASK);
    block->size = new_size | flags;
}

size_t block_get_size_prev(struct Header *block)
{
    return (block->size_prev & SIZE_MASK) * ALIGNMENT;
}
void block_set_size_prev(struct Header *block, size_t size)
{
    size_t new_size = (size / ALIGNMENT);
    size_t flags = block->size_prev & (~SIZE_MASK);
    block->size_prev = new_size | flags;
}

size_t block_get_addr(struct Header *block)
{
    return block->addr;
}
void block_set_addr(struct Header *block, size_t addr)
{
    block->addr = addr;
}

bool block_is_first(struct Header *block)
{
    return (block->size & FIRST_BLOCK) != 0;
}
bool block_is_last(struct Header *block)
{
    return (block->size_prev & LAST_BLOCK) != 0;
}
bool block_is_free(struct Header *block)
{
    return (block->size & FREE_BLOCK) != 0;
}
bool block_is_decommit(struct Header *block)
{
    return (block->size_prev & DECOMMIT_BLOCK) != 0;
}

void block_set_first(struct Header *block)
{
    block->size = block->size | FIRST_BLOCK;
}
void block_set_last(struct Header *block)
{
    block->size_prev = block->size_prev | LAST_BLOCK;
}
void block_set_free(struct Header *block)
{
    block->size = block->size | FREE_BLOCK;
}
void block_set_decommit(struct Header *block)
{
    block->size_prev = block->size_prev | DECOMMIT_BLOCK;
}

void block_unset_first(struct Header *block)
{
    block->size = block->size & ~FIRST_BLOCK;
}
void block_unset_last(struct Header *block)
{
    block->size_prev = block->size_prev & ~LAST_BLOCK;
}
void block_unset_free(struct Header *block)
{
    block->size = block->size & ~FREE_BLOCK;
}
void block_unset_decommit(struct Header *block)
{
    block->size_prev = block->size_prev & ~DECOMMIT_BLOCK;
}

struct Header *block_next(struct Header *block)
{
    if (block_is_last(block))
        return NULL;
    return (struct Header *)((char *)block + HEADER_SIZE + block_get_size_curr(block));
}
struct Header *block_prev(struct Header *block)
{
    if (block_is_first(block))
        return NULL;
    return (struct Header *)((char *)block - HEADER_SIZE - block_get_size_prev(block));
}

void create_header(struct Header *block, struct Header *prev, bool free, size_t size, size_t addr)
{
    block_set_size_curr(block, size);
    block_set_addr(block, addr);

    if (prev)
    {
        block_set_size_prev(block, block_get_size_curr(prev));
        block_unset_first(block);
    }
    else
        block_set_first(block);

    if (free)
        block_set_free(block);

    block_set_last(block);
}
