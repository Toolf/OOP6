#include "header.h"
#include <stdbool.h>

void create_header(struct Header *header, struct Header *prev, struct Header *next, bool free, size_t size)
{
    header->free = free;
    header->prev = prev;
    header->next = next;
    header->size = size;
}