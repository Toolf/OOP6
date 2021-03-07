#include <stdint.h>
#include "allocator.h"
#include "arena.h"

#define ALIGNMENT 8 /*in bytes*/
#define MAX_SIZE (1024 * ALIGNMENT)
#define HEADER_SIZE sizeof(struct Header)

static char default_arena[MAX_SIZE];

static struct Arena arena = {
    .body = default_arena,
    .started = 0,
};

struct Header
{
    uintptr_t prev;
    uintptr_t next;
    int size;
    char free;
};

inline void init_block(struct Header *ptr, size_t size, uintptr_t prev)
{
    ptr->prev = prev;
    ptr->next = NULL;
    ptr->size = size;
    ptr->free = 0;
}

void *
mem_alloc(size_t size)
{
    // ---------- alignment 64 bit
    size_t rem = size % ALIGNMENT;
    if (rem != 0)
        size += ALIGNMENT - rem;

    size += HEADER_SIZE;

    // ---------- find position of free block
    struct Header *header = (struct Header *)arena.body;
    if (arena.started)
    {
        // try to find free block that fits our size
        while (header->next && !header->free && header->size < size)
        {
            header = header->next;
        }
        // if not found
        if (!header->free && header->size < size)
        {
            // create new header
            header->next = ((char *)header) + header->size;
            init_block(header->next, size, header);
            header = header->next;
        }
    }
    else
    {
        // init start block
        init_block(header, size, NULL);
        arena.started = 1;
    }

    // ---------- calc user ptr
    char *ptr = (char *)header + HEADER_SIZE; // user ptr
    return ptr;
}

void mem_free(void *ptr)
{
    if (ptr == NULL)
        return;

    ((struct Header *)((char *)ptr - HEADER_SIZE))->free = 1;
}

void *mem_realloc(void *ptr, size_t new_size)
{
    if (!arena.started)
    {
        return NULL;
    }
    // check next header

    // this block has enought memory size return this ptr and update block
    // else
    // try to find suitable block and copy memory to it
    //  - if not next block expand this block
    //  - if next exist try to block which is free and is big enought
    //      - if not found create new block
    // free started block

    struct Header *block = (struct Header *)(((char *)ptr) - HEADER_SIZE);

    if (block->size > new_size)
    {
        block->size = new_size;
        // struct Header free_block = {
        //     .prev = block,
        //     .next = block->next,
        //     .size = block->size - new_size,
        //     .free = 1,
        // };
        return ptr;
    }
    else
    {
        if (!block->next)
        {
            block->size = new_size;
            return ptr;
        }
        else
        {
            struct Header *new_block = arena.body;
            while (new_block->next && !(new_block->free && new_block->size >= new_size))
            {
                new_block = new_block->next;
            }
            if (!(new_block->free && new_block->size >= new_size))
            {
                if (new_block->next)
                {
                    new_block = new_block->next;
                }
                else
                {
                    // TODO: not finished
                }
            }
        }
    }
}