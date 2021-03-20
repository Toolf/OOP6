#include <stddef.h>

#define ALIGNMENT _Alignof(max_align_t)

int square(int num)
{
    return ALIGNMENT;
}