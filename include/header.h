#pragma once

#include "config.h"
#include <stddef.h>
#include <stdbool.h>

#define HEADER_SIZE align(sizeof(struct Header))

struct Header
{
    struct Header *prev;
    struct Header *next;
    size_t size;
    bool free;
};

void create_header(struct Header *header, struct Header *prev, struct Header *next, bool free, size_t size);