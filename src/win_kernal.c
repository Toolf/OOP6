#pragma once

#include <windows.h>
#include <memoryapi.h>
#include "kernal.h"

static size_t page_size = 0;

static void get_page_size(void)
{
    if (page_size == 0)
    {
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        page_size = info.dwPageSize;
    }
    return page_size;
}

void *kernal_alloc(size_t size)
{
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void kernal_free(void *ptr, size_t size)
{
    VirtualFree(ptr, 0, MEM_RELEASE);
}

void *kernal_realloc(void *ptr, size_t new_size)
{
    // TODO : WIN Kernal realloc
    return NULL;
}