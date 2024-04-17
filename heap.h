//
// Created by root on 10/27/23.
//
#ifndef PROJECT1_HEAP_H
#define PROJECT1_HEAP_H

#include "custom_unistd.h"

#define STRUCT_SIZE sizeof(struct mem_block)
#define FENCE 8
#define PAGE_SIZE 4096
#define ALIGN_BYTES 1
#define MAHINE_WORD sizeof(void*)

enum pointer_type_t
{
    pointer_null,
    pointer_heap_corrupted,
    pointer_control_block,
    pointer_inside_fences,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};

struct mem_block{
    size_t checksum;
    int isfree;
    size_t size;
    size_t actual_size;
    struct mem_block* next;
    struct mem_block* prev;
    void* user_ptr;
    int fileline;
    const char* filename;
} __attribute__((__packed__));

struct mem_manager{
    int is_init;
    void* memory_start;
    size_t size;
    size_t left_free;
    struct mem_block* first_block;
} mMen;

int heap_setup(void);
void heap_clean(void);
void* heap_malloc(size_t size);
void* heap_calloc(size_t number, size_t size);
void* heap_realloc(void* memblock, size_t count);
void  heap_free(void* memblock);
enum pointer_type_t get_pointer_type(const void* const pointer);
int heap_validate(void);
size_t   heap_get_largest_used_block_size(void);

size_t setChecksum(struct mem_block* block);
int checkChecksum(struct mem_block* block);
int increaseHeapSize(size_t sizeNeeded);

void* heap_malloc_aligned(size_t count);
void* heap_calloc_aligned(size_t number, size_t size);
void* heap_realloc_aligned(void* memblock, size_t size);

void* heap_malloc_debug(size_t count, int fileline, const char* filename);
void* heap_calloc_debug(size_t number, size_t size, int fileline, const char* filename);
void* heap_realloc_debug(void* memblock, size_t size, int fileline, const char* filename);

void* heap_malloc_aligned_debug(size_t count, int fileline, const char* filename);
void* heap_calloc_aligned_debug(size_t number, size_t size, int fileline, const char* filename);
void* heap_realloc_aligned_debug(void* memblock, size_t size, int fileline, const char* filename);

#endif //PROJECT1_HEAP_H
