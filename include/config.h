#pragma once

#include <stddef.h>

// _Alignof(max_align_t), працює не для всіх компіляторів.
#define ALIGNMENT _Alignof(max_align_t)
#define align(N) align_by(N, ALIGNMENT)

static inline size_t align_by(size_t N, size_t align_by)
{
    return N % align_by == 0 ? N : N + (align_by - N % align_by);
}
