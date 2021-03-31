#ifndef KERNAL_H
#define KERNAL_H

#include <stddef.h>

size_t get_page_size(void);

void *kernal_alloc(size_t size);

void kernal_free(void *ptr, size_t size);

void kernal_reset(void *ptr, size_t size);

#endif /* KERNAL_H */