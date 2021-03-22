#pragma once

#include <stddef.h>

// _Alignof(max_align_t), працює не для всіх компіляторів.
#define ALIGNMENT _Alignof(max_align_t)

static inline size_t align(size_t N)
{
    return N % ALIGNMENT == 0 ? N : N + (ALIGNMENT - N % ALIGNMENT);
}