//
// Created by root on 10/27/23.
//
#include <stdint.h>
#include "heap.h"
#include <stdio.h>
#include "tested_declarations.h"
#include "rdebug.h"


size_t setChecksum(struct mem_block* block){
    block->checksum=0;
    block->checksum+=(size_t) block->isfree;
    block->checksum+=(size_t) block->size;
    block->checksum+=(size_t) block->actual_size;
    block->checksum+=(size_t) block->next;
    block->checksum+=(size_t) block->prev;
    block->checksum+=(size_t) block->user_ptr;
    block->checksum+=(size_t) block->filename;
    block->checksum+=(size_t) block->fileline;
    return block->checksum;
}
int checkChecksum(struct mem_block* block){
    size_t from_struct=block->checksum;
    size_t result= setChecksum(block);
    block->checksum=from_struct;
    if(block->checksum!=result) return 1;
    return 0;
}
int increaseHeapSize(size_t sizeNeeded){
    if(mMen.is_init==0) return 1;
    unsigned long pagesToAdd=1+sizeNeeded/PAGE_SIZE;
    void *temp= custom_sbrk(PAGE_SIZE*pagesToAdd);
    if(temp==((void*)-1)){
        return 1;
    }
    mMen.size+=pagesToAdd*PAGE_SIZE;
    mMen.left_free+=pagesToAdd*PAGE_SIZE;
    return 0;
}

int heap_setup(void){
    if(mMen.is_init==1) return 0;
    void *temp= custom_sbrk(PAGE_SIZE);
    if(temp==((void*)-1)){
        mMen.is_init=0;
        return -1;
    }
    mMen.memory_start=temp;
    mMen.size=PAGE_SIZE;
    mMen.left_free=PAGE_SIZE;
    mMen.is_init=1;
    mMen.first_block=NULL;
    return 0;
}
void heap_clean(void){
    if(mMen.is_init==0) return;
    custom_sbrk((long long int)mMen.size*(-1));
    mMen.is_init=0;
}
void* heap_malloc(size_t size){
    if(mMen.is_init!=1||size<1) {return NULL;}
    struct mem_block *temp=mMen.first_block;
    size_t actual_chunk_size=STRUCT_SIZE+size+2*FENCE;
    size_t shift_needed=0;
    if(temp==NULL){
        shift_needed=(MAHINE_WORD-((intptr_t)mMen.memory_start+STRUCT_SIZE+FENCE)%MAHINE_WORD)%MAHINE_WORD;
        if(actual_chunk_size+shift_needed>mMen.left_free){
            int result= increaseHeapSize(actual_chunk_size-mMen.left_free+PAGE_SIZE-STRUCT_SIZE-FENCE);
            if(result!=0) return NULL;
        }
        struct mem_block newBlock;
        newBlock.size=size;
        newBlock.isfree=0;
        newBlock.prev=NULL;
        newBlock.next=NULL;
        newBlock.user_ptr=NULL;
        newBlock.checksum=0;
        newBlock.actual_size=actual_chunk_size;
        *(struct mem_block*)((char*)mMen.memory_start+shift_needed)=newBlock;
        mMen.first_block=(struct mem_block*)((char *)mMen.memory_start+shift_needed);
        mMen.first_block->user_ptr=(char*)mMen.first_block+STRUCT_SIZE+FENCE;
        setChecksum(mMen.first_block);
        for (int i = 0; i < FENCE; ++i) {
            *((char*)mMen.first_block+ STRUCT_SIZE+i)='#';
            *((char*)mMen.first_block->user_ptr + size + i)='#';
        }
        mMen.left_free-=mMen.first_block->actual_size+shift_needed;
        return mMen.first_block->user_ptr;
    }
    //to check
    if((size_t)((intptr_t)mMen.first_block-(intptr_t)mMen.memory_start)>=actual_chunk_size){
        struct mem_block newBlock;
        newBlock.size=size;
        newBlock.isfree=0;
        newBlock.prev=NULL;
        newBlock.next=temp;
        newBlock.user_ptr=NULL;
        newBlock.checksum=0;
        newBlock.actual_size=(size_t)((intptr_t)mMen.first_block-(intptr_t)mMen.memory_start);
        *(struct mem_block*)((char*)mMen.memory_start)=newBlock;
        mMen.first_block->prev=(struct mem_block*)mMen.memory_start;
        setChecksum(mMen.first_block);
        mMen.first_block=(struct mem_block*)((char *)mMen.memory_start);
        mMen.first_block->user_ptr=(char*)mMen.first_block+STRUCT_SIZE+FENCE;
        setChecksum(mMen.first_block);
        for (int i = 0; i < FENCE; ++i) {
            *((char*)mMen.first_block+ STRUCT_SIZE+i)='#';
            *((char*)mMen.first_block->user_ptr + size + i)='#';
        }
        return mMen.first_block->user_ptr;
    }
    for (; temp->next !=NULL; temp=temp->next){
        shift_needed=(MAHINE_WORD-((intptr_t)temp+temp->size+2*STRUCT_SIZE+3*FENCE)%MAHINE_WORD)%MAHINE_WORD;
        if(temp->isfree==1&&((intptr_t )temp->user_ptr%MAHINE_WORD)==0){
            if(temp->actual_size<actual_chunk_size){
                continue;
            }
            temp->isfree=0;
            temp->size=size;
            for (int i = 0; i < FENCE; ++i) {
                *((char*)temp+ STRUCT_SIZE+i)='#';
                *((char*)temp->user_ptr+size+i)='#';
            }
            setChecksum(temp);
            return temp->user_ptr;
        }
        //It creates a new block in the list if there is a used block with the actual size bigger than user data, and the new block fits of course
        if(temp->actual_size-temp->size-STRUCT_SIZE-2*FENCE>=actual_chunk_size+shift_needed){
            struct mem_block newBlock;
            newBlock.size=size;
            newBlock.isfree=0;
            newBlock.prev=temp;
            newBlock.next=NULL;
            newBlock.user_ptr=NULL;
            newBlock.checksum=0;
            newBlock.actual_size=temp->actual_size-temp->size-shift_needed-STRUCT_SIZE-2*FENCE;
            temp->next->prev=(struct mem_block*)((char*)temp+temp->size+STRUCT_SIZE+2*FENCE+shift_needed);
            setChecksum(temp->next);
            newBlock.next=temp->next;
            temp->next=(struct mem_block*)((char*)temp+temp->size+STRUCT_SIZE+2*FENCE+shift_needed);
            temp->actual_size-=newBlock.actual_size;
            setChecksum(temp);
            *temp->next=newBlock;
            temp->next->user_ptr=(char*)temp->next+STRUCT_SIZE+FENCE;
            setChecksum(temp->next);
            for (int i = 0; i < FENCE; ++i) {
                *((char*)temp->next+ STRUCT_SIZE+i)='#';
                *((char*)temp->next->user_ptr+size+i)='#';
            }
            return temp->next->user_ptr;
        }
    }
    shift_needed=(MAHINE_WORD-((intptr_t)temp+temp->actual_size+STRUCT_SIZE+FENCE)%MAHINE_WORD)%MAHINE_WORD;
    if(actual_chunk_size+shift_needed>mMen.left_free){
        int result= increaseHeapSize(actual_chunk_size+shift_needed-mMen.left_free);
        if(result!=0) return NULL;
    }
    struct mem_block newBlock;
    newBlock.size=size;
    newBlock.isfree=0;
    newBlock.prev=temp;
    newBlock.next=NULL;
    newBlock.user_ptr=NULL;
    newBlock.checksum=0;
    newBlock.actual_size=actual_chunk_size;
    temp->actual_size+=shift_needed;
    mMen.left_free-=shift_needed;
    temp->next=(struct mem_block*)((char*)temp+ temp->actual_size);
    setChecksum(temp);
    *(struct mem_block*)((char*)temp+ temp->actual_size)=newBlock;
    for (int i = 0; i < FENCE; ++i) {
        *((char*)temp->next+ STRUCT_SIZE+i)='#';
        *((char*)temp->next+ STRUCT_SIZE+FENCE+size+i)='#';
    }
    temp->next->user_ptr=(char*)temp->next+STRUCT_SIZE+FENCE;
    setChecksum(temp->next);
    mMen.left_free-=actual_chunk_size;
    return temp->next->user_ptr;
}
void* heap_calloc(size_t number, size_t size){
    if(number==0||size==0) return NULL;
    void* result= heap_malloc(size*number);
    if(result==NULL) return NULL;
    struct mem_block* temp=(struct mem_block*)((char *)result-FENCE-STRUCT_SIZE);
    for (int i = 0; i < (int)temp->size; ++i) {
        *((char*)temp->user_ptr+i)=0;
    }
    return result;
}
void* heap_realloc(void* memblock, size_t count){
    if(heap_validate()) return NULL;
    if(memblock==NULL){
        return heap_malloc(count);
    }
    if(count<1&& get_pointer_type(memblock)==pointer_valid) {
        heap_free(memblock);
        return NULL;
    }
    if(get_pointer_type(memblock)!=pointer_valid) return NULL;
    struct mem_block* temp=(struct mem_block*)((char *)memblock-FENCE-STRUCT_SIZE);
    if(temp->isfree) return NULL;
    if(temp->size==count) return memblock;
    if(temp->size>count){
        temp->size=count;
        setChecksum(temp);
        for (int i = 0; i < FENCE; ++i) {
            *((char*)temp+STRUCT_SIZE+FENCE+i+temp->size)='#';
        }
        return temp->user_ptr;
    }
    size_t spaceInCurr=temp->actual_size-STRUCT_SIZE-2*FENCE;
    if(spaceInCurr>=count){
        temp->size=count;
        setChecksum(temp);
        for (int i = 0; i < FENCE; ++i) {
            *((char*)temp->user_ptr+i+temp->size)='#';
        }
        return temp->user_ptr;
    }
    size_t actual_chunk_size=STRUCT_SIZE+count+2*FENCE;
    //not used if the align is 1
    if(actual_chunk_size%ALIGN_BYTES!=0){
        actual_chunk_size+=ALIGN_BYTES-actual_chunk_size%ALIGN_BYTES;
    }
    if(temp->next==NULL){
        if(mMen.left_free<actual_chunk_size-temp->actual_size){
            if(increaseHeapSize(actual_chunk_size-temp->actual_size)) return NULL;
        }
        mMen.left_free-=actual_chunk_size-temp->actual_size;
        temp->actual_size=actual_chunk_size;
        temp->size=count;
        setChecksum(temp);
        for (int i = 0; i < FENCE; ++i) {
            *((char*)temp+ STRUCT_SIZE+FENCE+i+temp->size)='#';
        }
        return temp->user_ptr;
    }
    struct mem_block savedBlock=*temp->next;
    //I make the next block that is free get eaten by the block which needs more space(A=500,B is free=200)->realloc(A,550)->(A size=550 actual size=700)
    //sketchy idk if dante wants me to do this EDIT: dante wants me to do this
    if(temp->next->isfree&&(temp->actual_size+temp->next->actual_size>=actual_chunk_size)){
        temp->next=temp->next->next;
        temp->next->prev=temp;
        setChecksum(temp->next);
        temp->actual_size=temp->actual_size+savedBlock.actual_size;
        temp->size=count;
        for (int i = 0; i < FENCE; ++i) {
            *((char*)temp->user_ptr+i+temp->size)='#';
        }
        setChecksum(temp);
        return temp->user_ptr;
    }
    void* result= heap_malloc(count);
    if(result==NULL) return NULL;
    for (int i = 0; i < (int)temp->size; ++i) {
        *((char*)result+i)=*((char*)temp->user_ptr+i);
    }
    heap_free(memblock);
    return result;
}
//could be prettier
void  heap_free(void* memblock){
    if(get_pointer_type(memblock)!=pointer_valid) {
        return;
    }
    struct mem_block* temp=(struct mem_block*)((char*)memblock-STRUCT_SIZE-FENCE);
    temp->isfree=1;
    setChecksum(temp);
    if(temp->next==NULL) {
        if(temp->prev==NULL){
            mMen.first_block=NULL;
            mMen.left_free=mMen.size;
        } else {
            if(temp->prev->isfree==1){
                if (temp->prev->prev==NULL){
                    mMen.first_block=NULL;
                    mMen.left_free=mMen.size;
                } else {
                    temp->prev->prev->next=NULL;
                    setChecksum(temp->prev->prev);
                    mMen.left_free=mMen.left_free+temp->actual_size+temp->prev->actual_size;
                }
            } else {
                temp->prev->next=NULL;
                setChecksum(temp->prev);
                mMen.left_free+=temp->actual_size;
            }
        }
        return;
    }
    if(temp->next->isfree==1){
        temp->actual_size+=temp->next->actual_size;
        setChecksum(temp);
        temp->next->next->prev=temp;
        setChecksum(temp->next->next);
        temp->next=temp->next->next;
        setChecksum(temp);
        //got a bit confusing because of the line above, careful there

    }
    if(temp->prev!=NULL&&temp->prev->isfree){
        temp->prev->actual_size+=temp->actual_size;
        temp->next->prev=temp->prev;
        setChecksum(temp->next);
        temp->prev->next=temp->next;
        setChecksum(temp->prev);
    } /*else if(temp->prev==NULL&&((char *)mMen.first_block!=(char*)mMen.memory_start)){
        struct mem_block old_struct=*temp;
        *(struct mem_block*)mMen.memory_start=old_struct;
        mMen.first_block=mMen.memory_start;
        mMen.first_block->actual_size+=4096-STRUCT_SIZE-FENCE;
        temp->next->prev=mMen.first_block;
        setChecksum(temp->next);
        setChecksum(mMen.first_block);
    }*/
}
enum pointer_type_t get_pointer_type(const void* const pointer){
    if(pointer==NULL) return pointer_null;
    if(heap_validate()!=0) {
        return pointer_heap_corrupted;
    }
    for (struct mem_block* temp = mMen.first_block; temp !=NULL; temp=temp->next){
        if((char *)pointer>=(char*)temp&&(char *)pointer<(char*)temp+STRUCT_SIZE) return pointer_control_block;
        if(temp->isfree==0) {
            if ((char *) pointer >= (char *) temp + STRUCT_SIZE &&
                (char *) pointer < (char *) temp + STRUCT_SIZE + FENCE)
                return pointer_inside_fences;
            if ((char *) pointer == (char *) temp->user_ptr) return pointer_valid;
            if ((char *) pointer > (char *) temp + STRUCT_SIZE + FENCE &&
                (char *) pointer < (char *) temp + STRUCT_SIZE + FENCE + temp->size)
                return pointer_inside_data_block;
            if ((char *) pointer >= (char *) temp + STRUCT_SIZE + FENCE + temp->size &&
                (char *) pointer < (char *) temp + STRUCT_SIZE + 2 * FENCE + temp->size)
                return pointer_inside_fences;
        }
    }
    return pointer_unallocated;
}
int heap_validate(void){
    if(mMen.is_init!=1) return 2;
    for (struct mem_block* temp = mMen.first_block; temp !=NULL; temp=temp->next) {
        if (checkChecksum(temp)){
            return 3;
        }
        if(temp->isfree==1) continue;
        for (int i = 0; i < FENCE; ++i) {
            if(*((char*)temp+STRUCT_SIZE+i)!='#') {
                return 1;
            }
            if(*((char*)temp->user_ptr+temp->size+i)!='#') {
                return 1;
            }
        }
    }
    return 0;
}
size_t   heap_get_largest_used_block_size(void){
    if(heap_validate()!=0) return 0;
    size_t result=0;
    for (struct mem_block* temp = mMen.first_block; temp !=NULL; temp=temp->next){
        if(temp->isfree!=1&&result<temp->size){
            result=temp->size;
        }
    }
    return result;
}
void* heap_malloc_aligned(size_t count){
    if(mMen.is_init!=1 || count < 1) {return NULL;}
    struct mem_block *temp=mMen.first_block;
    size_t actual_chunk_size= STRUCT_SIZE + count + 2 * FENCE;
    if(temp==NULL){
        /*if(((intptr_t)mMen.memory_start & (intptr_t)(PAGE_SIZE - 1)) != 0){
            printf("##memory not aligned##");
        }*/
        if(actual_chunk_size+PAGE_SIZE-STRUCT_SIZE-FENCE>mMen.left_free){
            int result= increaseHeapSize(actual_chunk_size-mMen.left_free+PAGE_SIZE-STRUCT_SIZE-FENCE);
            if(result!=0) return NULL;
        }
        struct mem_block newBlock;
        newBlock.size=count;
        newBlock.isfree=0;
        newBlock.prev=NULL;
        newBlock.next=NULL;
        newBlock.user_ptr=NULL;
        newBlock.checksum=0;
        newBlock.actual_size=actual_chunk_size;
        *(struct mem_block*)((char*)mMen.memory_start+4096-STRUCT_SIZE-FENCE)=newBlock;
        mMen.first_block=(struct mem_block*)((char *)mMen.memory_start+4096-STRUCT_SIZE-FENCE);
        mMen.first_block->user_ptr=(char*)mMen.first_block+STRUCT_SIZE+FENCE;
        setChecksum(mMen.first_block);
        for (int i = 0; i < FENCE; ++i) {
            *((char*)mMen.first_block+ STRUCT_SIZE+i)='#';
            *((char*)mMen.first_block->user_ptr + count + i)='#';
        }
        mMen.left_free-=mMen.first_block->actual_size+4096-STRUCT_SIZE-FENCE;
        return mMen.first_block->user_ptr;
    }
    size_t shift_needed=0;
    for (; temp->next !=NULL; temp=temp->next){
        //shift_needed=(PAGE_SIZE-((intptr_t)temp+temp->size+2*STRUCT_SIZE+3*FENCE)%PAGE_SIZE)%PAGE_SIZE;
        if(temp->isfree==1&&(((intptr_t)temp->user_ptr & (intptr_t)(PAGE_SIZE - 1))==0)){
            if(temp->actual_size<actual_chunk_size){
                continue;
            }
            temp->isfree=0;
            temp->size=count;
            for (int i = 0; i < FENCE; ++i) {
                *((char*)temp+ STRUCT_SIZE+i)='#';
                *((char*)temp->user_ptr + count + i)='#';
            }
            setChecksum(temp);
            return temp->user_ptr;
        }

    }
    shift_needed=(PAGE_SIZE-((intptr_t)temp+temp->actual_size+STRUCT_SIZE+FENCE)%PAGE_SIZE)%PAGE_SIZE;
    if(actual_chunk_size+shift_needed>mMen.left_free){
        int result= increaseHeapSize(actual_chunk_size+shift_needed-mMen.left_free);
        if(result!=0) return NULL;
    }
    struct mem_block newBlock;
    newBlock.size=count;
    newBlock.isfree=0;
    newBlock.prev=temp;
    newBlock.next=NULL;
    newBlock.user_ptr=NULL;
    newBlock.checksum=0;
    newBlock.actual_size=actual_chunk_size;
    temp->actual_size+=shift_needed;
    mMen.left_free-=shift_needed;
    temp->next=(struct mem_block*)((char*)temp+ temp->actual_size);
    setChecksum(temp);
    *(struct mem_block*)((char*)temp+ temp->actual_size)=newBlock;
    for (int i = 0; i < FENCE; ++i) {
        *((char*)temp->next+ STRUCT_SIZE+i)='#';
        *((char*)temp->next + STRUCT_SIZE + FENCE + count + i)='#';
    }
    temp->next->user_ptr=(char*)temp->next+STRUCT_SIZE+FENCE;
    setChecksum(temp->next);
    mMen.left_free-=actual_chunk_size;
    return temp->next->user_ptr;
}
void* heap_calloc_aligned(size_t number, size_t size){
    if(number==0||size==0) return NULL;
    void* result= heap_malloc_aligned(size*number);
    if(result==NULL) return NULL;
    struct mem_block* temp=(struct mem_block*)((char *)result-FENCE-STRUCT_SIZE);
    for (int i = 0; i < (int)temp->size; ++i) {
        *((char*)temp->user_ptr+i)=0;
    }
    return result;
}
void* heap_realloc_aligned(void* memblock, size_t size){
    if(heap_validate()) return NULL;
    if(memblock==NULL){
        return heap_malloc_aligned(size);
    }
    if(size < 1 && get_pointer_type(memblock) == pointer_valid) {
        heap_free(memblock);
        return NULL;
    }
    if(get_pointer_type(memblock)!=pointer_valid) return NULL;
    struct mem_block* temp=(struct mem_block*)((char *)memblock-FENCE-STRUCT_SIZE);
    if(temp->isfree) return NULL;
    if(((intptr_t)memblock & (intptr_t)(PAGE_SIZE - 1)) != 0){
        void* result= heap_malloc_aligned(size);
        if(result==NULL) return NULL;
        for (int i = 0; i < (int)temp->size; ++i) {
            *((char*)result+i)=*((char*)temp->user_ptr+i);
        }
        heap_free(memblock);
        return result;
    }
    if(temp->size == size) return memblock;
    if(temp->size > size){
        temp->size=size;
        setChecksum(temp);
        for (int i = 0; i < FENCE; ++i) {
            *((char*)temp+STRUCT_SIZE+FENCE+i+temp->size)='#';
        }
        return temp->user_ptr;
    }
    size_t spaceInCurr=temp->actual_size-STRUCT_SIZE-2*FENCE;
    if(spaceInCurr >= size){
        temp->size=size;
        setChecksum(temp);
        for (int i = 0; i < FENCE; ++i) {
            *((char*)temp->user_ptr+i+temp->size)='#';
        }
        return temp->user_ptr;
    }
    size_t actual_chunk_size= STRUCT_SIZE + size + 2 * FENCE;
    //not used if the align is 1
    if(actual_chunk_size%ALIGN_BYTES!=0){
        actual_chunk_size+=ALIGN_BYTES-actual_chunk_size%ALIGN_BYTES;
    }
    if(temp->next==NULL){
        if(mMen.left_free<actual_chunk_size-temp->actual_size){
            if(increaseHeapSize(actual_chunk_size-temp->actual_size)) return NULL;
        }
        mMen.left_free-=actual_chunk_size-temp->actual_size;
        temp->actual_size=actual_chunk_size;
        temp->size=size;
        setChecksum(temp);
        for (int i = 0; i < FENCE; ++i) {
            *((char*)temp+ STRUCT_SIZE+FENCE+i+temp->size)='#';
        }
        return temp->user_ptr;
    }
    struct mem_block savedBlock=*temp->next;
    //I make the next block that is free get eaten by the block which needs more space(A=500,B is free=200)->realloc(A,550)->(A size=550 actual size=700)
    //sketchy idk if dante wants me to do this EDIT: dante wants me to do this
    if(temp->next->isfree&&(temp->actual_size+temp->next->actual_size>=actual_chunk_size)){
        temp->next=temp->next->next;
        temp->next->prev=temp;
        setChecksum(temp->next);
        temp->actual_size=temp->actual_size+savedBlock.actual_size;
        temp->size=size;
        for (int i = 0; i < FENCE; ++i) {
            *((char*)temp->user_ptr+i+temp->size)='#';
        }
        setChecksum(temp);
        return temp->user_ptr;
    }
    void* result= heap_malloc_aligned(size);
    if(result==NULL) return NULL;
    for (int i = 0; i < (int)temp->size; ++i) {
        *((char*)result+i)=*((char*)temp->user_ptr+i);
    }
    heap_free(memblock);
    return result;
}
void* heap_malloc_debug(size_t count, int fileline, const char* filename){
    void* result= heap_malloc(count);
    if(result==NULL) return NULL;
    struct mem_block* temp=(struct mem_block*)((char*)result-STRUCT_SIZE-FENCE);
    temp->fileline=fileline;
    temp->filename=filename;
    setChecksum(temp);
    return result;
}
void* heap_calloc_debug(size_t number, size_t size, int fileline, const char* filename){
    void* result= heap_calloc(number,size);
    if(result==NULL) return NULL;
    struct mem_block* temp=(struct mem_block*)((char*)result-STRUCT_SIZE-FENCE);
    temp->fileline=fileline;
    temp->filename=filename;
    setChecksum(temp);
    return result;
}
void* heap_realloc_debug(void* memblock, size_t size, int fileline, const char* filename){
    void* result= heap_realloc(memblock,size);
    if(result==NULL) return NULL;
    struct mem_block* temp=(struct mem_block*)((char*)result-STRUCT_SIZE-FENCE);
    temp->fileline=fileline;
    temp->filename=filename;
    setChecksum(temp);
    return result;
}

void* heap_malloc_aligned_debug(size_t count, int fileline, const char* filename){
    void* result= heap_malloc_aligned(count);
    if(result==NULL) return NULL;
    struct mem_block* temp=(struct mem_block*)((char*)result-STRUCT_SIZE-FENCE);
    temp->fileline=fileline;
    temp->filename=filename;
    setChecksum(temp);
    return result;
}
void* heap_calloc_aligned_debug(size_t number, size_t size, int fileline, const char* filename){
    void* result= heap_calloc_aligned(number,size);
    if(result==NULL) return NULL;
    struct mem_block* temp=(struct mem_block*)((char*)result-STRUCT_SIZE-FENCE);
    temp->fileline=fileline;
    temp->filename=filename;
    setChecksum(temp);
    return result;
}
void* heap_realloc_aligned_debug(void* memblock, size_t size, int fileline, const char* filename){
    void* result= heap_realloc_aligned(memblock,size);
    if(result==NULL) return NULL;
    struct mem_block* temp=(struct mem_block*)((char*)result-STRUCT_SIZE-FENCE);
    temp->fileline=fileline;
    temp->filename=filename;
    setChecksum(temp);
    return result;
}

