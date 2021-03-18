#pragma once

#include <stddef.h>
#include <stdbool.h>

// #pragma pack(push)      /* push current alignment to stack */
// #pragma pack(ALIGNMENT) /* set alignment to 1 byte boundary */

struct Header
{
    struct Header *prev;
    struct Header *next;
    // size_t prev_size;
    // size_t next_size;
    size_t size;
    bool free;
};

// #pragma pack(pop)

void create_header(struct Header *header, struct Header *prev, struct Header *next, bool free, size_t size);