#include <stddef.h>

void *kernal_alloc(size_t size);

void kernal_free(void *ptr, size_t size);

void *kernal_realloc(void *ptr, size_t new_size);

size_t get_page_size(void);