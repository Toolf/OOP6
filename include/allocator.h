#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h> // have size_t and NULL

/*
 * Return pointer to allocated memory
 * Return NULL if can't allocate memory 
 */
void *mem_alloc(size_t size);

/*
 * Free alocated memory by pointer
 */
void mem_free(void *ptr);

/*
 * Reallocates pointer to new size and return new pointer
 */
void *mem_realloc(void *ptr, size_t new_size);

void mem_print(void);

#endif /* ALLOCATOR_H */