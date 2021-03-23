#pragma once

#include "config.h"
#include <stddef.h>
#include <stdbool.h>

#define HEADER_SIZE align(sizeof(struct Header))

struct Header
{
    size_t size;      // кількість _Alignof(max_align_t)
    size_t size_prev; // кількість _Alignof(max_align_t)
    size_t addr;      // відносна адресав, відносно початку арени. Потрібна для decommit та commit.
};

size_t block_get_size_curr(struct Header *);
void block_set_size_curr(struct Header *, size_t size);
size_t block_get_size_prev(struct Header *);
void block_set_size_prev(struct Header *, size_t prev);
size_t block_get_addr(struct Header *);
void block_set_addr(struct Header *, size_t addr);

bool block_is_first(struct Header *);
bool block_is_last(struct Header *);
bool block_is_free(struct Header *);
bool block_is_decommit(struct Header *);

void block_set_first(struct Header *);
void block_set_last(struct Header *);
void block_set_free(struct Header *);
void block_set_decommit(struct Header *);

void block_unset_first(struct Header *);
void block_unset_last(struct Header *);
void block_unset_free(struct Header *);
void block_unset_decommit(struct Header *);

struct Header *block_next(struct Header *block);
struct Header *block_prev(struct Header *block);

void create_header(struct Header *block, struct Header *prev, bool free, size_t size, size_t addr);