#include <stdio.h>
#include "heap.h"
#include "custom_unistd.h"
#include "tested_declarations.h"
#include "rdebug.h"

int main() {
    heap_setup();
    heap_get_largest_used_block_size();
    heap_validate();
    heap_malloc(0);
    heap_calloc(0,0);
    heap_realloc(NULL,0);
    heap_free(NULL);
    heap_malloc_aligned(0);
    heap_calloc_aligned(0,0);
    heap_realloc_aligned(NULL,0);
    heap_clean();
    return 0;
}

