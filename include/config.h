#include <stddef.h>

// TODO : _Alignof(max_align_t), чогось не працює.
#define ALIGNMENT 5
#define align(N) (N % ALIGNMENT == 0 ? N : N + (ALIGNMENT - N % ALIGNMENT))