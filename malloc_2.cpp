#include <cstdlib>
#include <ctime>
#include <cmath>
#include <unistd.h>
#include <cstring>
#include <sys/mman.h>
#include <iostream>

struct Block_Node {
    size_t size;
    bool is_free;
    Block_Node* next;
    Block_Node* prev;
};

Block_Node* list_head = nullptr;
size_t counter_free_blocks = 0;
size_t counter_free_bytes = 0;
size_t counter_allocated_blocks = 0;
size_t counter_allocated_bytes = 0;
size_t counter_meta_data_bytes = 0;


void* smalloc(size_t size){
    if(size == 0 || size > 1e8){
        return NULL;
    }
    Block_Node* current = list_head;
    while(current != NULL){
        if(current->is_free && current->size >= size){
            current->is_free = false;
            counter_free_blocks--;
            counter_free_bytes -= current->size;
            return (char*)(current) + sizeof(Block_Node);
        }
        current = current->next;
    }
    void* request = sbrk(size + sizeof(Block_Node));
    if(request == (void*)(-1)){//Nizan- moved above
        return NULL;
    }
    counter_allocated_blocks++;
    counter_allocated_bytes += size;
    counter_meta_data_bytes += sizeof(Block_Node);
    Block_Node* new_block = (Block_Node*)request;
    new_block->size = size;
    new_block->is_free = false;
    new_block->next = NULL;
    new_block->prev = NULL;
    //TESTS: wrote this again, old one not good, switch
    current = list_head;
    if (list_head == nullptr) {
        list_head = new_block;
    }

        // If the new node should be inserted at the beginning
    else if (new_block < list_head) {
        new_block->next = list_head;
        list_head->prev = new_block;
        list_head = new_block;
    }

    else {
        // Find the correct position to insert the new node
        while (current->next != nullptr && current->next < new_block) {
            current = current->next;
        }

        // Insert the new node
        new_block->next = current->next;
        new_block->prev = current;
        if (current->next != nullptr) {
            current->next->prev = new_block;
        }
        current->next = new_block;

    }

    size_t s = sizeof(Block_Node);

    return (char*)new_block + s; //TESTS : char* for bytes and than and num of bytes
}

void* scalloc(size_t num, size_t size){
    if (size * num > pow(10,8))
    {
        return NULL;
    }
    void* request = smalloc(num * size);
    if(request == NULL){
        return NULL;
    }
    memset(request, 0, num * size);
    return request;
}

void sfree(void* p){
    if(p == NULL){
        return;
    }
    Block_Node* block = (Block_Node*)((char*)p - sizeof(Block_Node)); //TESTS : char* for bytes
    block->is_free = true;
    counter_free_blocks++;
    counter_free_bytes += block->size;
}

void* srealloc(void* oldp, size_t size){
    if(size == 0 || size > pow(10,8)){
        return NULL;
    }
    if(oldp == NULL){
        return smalloc(size);
    }
    Block_Node* block = (Block_Node*)((char*)oldp - sizeof(Block_Node));
    if(block->size >= size){
        return oldp;
    }
    void* newp = smalloc(size ); //TEST: not need to allocate more
    if(newp == NULL){
        return NULL;
    }
    memmove(newp, oldp, block->size);
    sfree(oldp);
    //free balance stays
//    return newp + sizeof(Block_Node);//nizan
    return newp;
}

size_t _num_free_blocks(){
    return counter_free_blocks;
}
size_t _num_free_bytes(){
    return counter_free_bytes;
}
size_t _num_allocated_blocks(){
    return counter_allocated_blocks;
}
size_t _num_allocated_bytes(){
    return counter_allocated_bytes;
}
size_t _num_meta_data_bytes(){
    return counter_meta_data_bytes;
}
size_t _size_meta_data(){
    return sizeof(Block_Node);
}
