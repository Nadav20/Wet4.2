#include <cstdlib>
#include <ctime>
#include <cmath>
#include <unistd.h>
#include <cstring>
#include <string>
#include <sys/mman.h>
#include <iostream>
#include <vector>
using namespace std;


struct MallocMetadata {
    size_t canary; //Where should the canary be placed in the metadata?
    bool is_free;
    size_t size;//actual size of the whole block minus the metadata
    MallocMetadata* next;
    MallocMetadata* prev;
};

//std::srand(std::time(nullptr));//not working outside a function (nizan)
size_t ourCanary;
const int max_order = 10;
bool first_time = true;
MallocMetadata* free_lists[max_order + 1];
size_t num_free_blocks = 0;
size_t num_free_bytes = 0;
size_t num_allocated_blocks = 0;
size_t num_allocated_bytes = 0;
size_t num_meta_data_bytes = 0;
MallocMetadata* mmap_region = nullptr;

MallocMetadata* find_buddy(MallocMetadata* block){
    unsigned long address = (unsigned long)block ^ (block->size + sizeof(MallocMetadata));
    return (MallocMetadata*)address;
}

int find_order(MallocMetadata* block){
    return log2((double)(block->size + sizeof(MallocMetadata))) - 7;
}

void check_meta_data(MallocMetadata *block){
    if(block->canary != ourCanary){ // if the canary is not equal to our canary
        exit(0xdeadbeef);
    }
}


void remove_block_from_free_list(MallocMetadata* block){
    check_meta_data(block);
    MallocMetadata* first = free_lists[find_order(block)], *curr = first;
    if(first == nullptr){
        return;
    }
    else{
        while(curr != block){
            curr = curr->next;
        }
        if(curr == first){ //curr is the first block
            //first = NULL;Nizan2
            free_lists[find_order(block)] = curr->next;
            if(curr->next != nullptr){
                curr->next->prev = curr->prev;
            }
        }
        else{
            check_meta_data(curr);
            curr->prev->next = curr->next;
            if(curr->next != nullptr){
                curr->next->prev = curr->prev;
            }
        }
    }
    block->next = nullptr;
    block->prev = nullptr;
}


void enter_block_into_free_list(MallocMetadata* new_block){
    int order = find_order(new_block);
    new_block->canary = ourCanary;
    MallocMetadata* first = free_lists[order];
    MallocMetadata* current = first;
    /////////
    //TESTS: wrote this again, old one not good, switch
    current = first;
    if (first == nullptr) {
        free_lists[order] = new_block;
    }

        // If the new node should be inserted at the beginning
    else if (new_block < first) {
        new_block->next = first;
        first->prev = new_block;
        new_block->prev = nullptr;
        free_lists[order] = new_block;
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
    ////////

}

bool try_to_merge(MallocMetadata* block, int required_order = max_order){
    check_meta_data(block);
    int order = find_order(block);
    if(order >= required_order){
        return false;
    }
    MallocMetadata* buddy = find_buddy(block);
    if(buddy->size != block->size){
        return false;
    }
    check_meta_data(buddy); //check if buddy is free
    if(buddy->is_free){
        remove_block_from_free_list(buddy);
        remove_block_from_free_list(block); //remove block from free list
        if(buddy < block){////latest
            block = buddy;
        }
        block->size = (block->size + sizeof(MallocMetadata)) * 2 - sizeof(MallocMetadata);
        enter_block_into_free_list(block);
        num_free_blocks--;
        num_free_bytes += sizeof(MallocMetadata);
        num_allocated_blocks--;
        num_allocated_bytes += sizeof(MallocMetadata);
        num_meta_data_bytes -= sizeof(MallocMetadata);
        return true;
    }
    return false;
}



int try_to_split(MallocMetadata* new_block, size_t requestedSize){
    check_meta_data(new_block);
    int order = find_order(new_block);
    size_t size_after_split = (new_block->size + sizeof(MallocMetadata)) / 2 - sizeof(MallocMetadata);
    if(order <= 0 || size_after_split < requestedSize){
        return -1;
    }
    remove_block_from_free_list(new_block);//is it affecting num_free_blocks?
    new_block->size = size_after_split;
    MallocMetadata* buddy = find_buddy(new_block);
    buddy->size = new_block->size;
    buddy->is_free = true;
    new_block->canary = ourCanary;
    buddy->canary = ourCanary;
    enter_block_into_free_list(buddy);
    enter_block_into_free_list(new_block);
    num_free_blocks += 1;
    num_free_bytes -= sizeof(MallocMetadata);
    num_allocated_blocks++;
    num_allocated_bytes -= sizeof(MallocMetadata);
    num_meta_data_bytes += sizeof(MallocMetadata);
    return order - 1;
}



void* initialize_stuff(){
    //initialize the canary
    std::srand(std::time(nullptr));
    ourCanary = std::rand();

    //align the memory
    auto address = (intptr_t)sbrk(0);
    intptr_t remainder = address % (32 * 128 * 1024);
    intptr_t bytes_from_allignement = (remainder == 0) ? 0 : (intptr_t)(32 * 128 * 1024) - remainder;
    intptr_t* request = (intptr_t*)sbrk(bytes_from_allignement + 32 * 128 * 1024);
    //auto new_address = (intptr_t)request; ///WHY? YOU DONT USE IT
    if(request == (void*)(-1)){
        return (void*)(-1);
    }

    //initialize the free lists
    for(auto & free_list : free_lists){
        free_list = nullptr;
    }

    auto curr = (MallocMetadata*)((char*)address + bytes_from_allignement);
    for (int i = 0; i < 32; i++) {
        curr->size = 128 * 1024 - sizeof(MallocMetadata);
        curr->canary = ourCanary;
        curr->is_free = true;
        curr->next = nullptr;
        curr->prev = nullptr;
        enter_block_into_free_list(curr);
        curr = (MallocMetadata*) ((char*) curr + 128 * 1024);
        num_free_blocks++;
        num_free_bytes += 128 * 1024 - sizeof(MallocMetadata);
        num_allocated_blocks++;
        num_allocated_bytes += 128 * 1024 - sizeof(MallocMetadata);
        num_meta_data_bytes += sizeof(MallocMetadata);
    }
    return nullptr;
}


void* smalloc(size_t size){
    //gets the size from the user-so the exact size the user wanted without extra stuff
    if(first_time){
        first_time = false;
        if(initialize_stuff() == (void*)(-1)){
            return nullptr;
        }
    }

    if(size == 0 || size > 1e8){
        return nullptr;
    }

    //mmap part
    if(size + sizeof(MallocMetadata) >= 1024 * 128){
        //use mmap
        void* request = mmap(NULL, size + sizeof(MallocMetadata), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if(request == (void*)(-1)){
            return nullptr;
        }
        MallocMetadata* block = (MallocMetadata*)request;
        block->size = size;
        block->canary = ourCanary;
        block->is_free = false;
        //add to mmap_region
//        while(curr != nullptr && block < curr){
//            curr = curr->next;
//        }
//        if(curr == nullptr){
//            block->next = nullptr;
//            mmap_region = block;
//        }
//        else{
//            block->next = curr;
//            block->prev = curr->prev;
//            curr->prev->next = block;
//            curr->prev = block;
//        }
//        curr->next = nullptr;
//        curr->prev = nullptr;
        num_allocated_blocks++;
        num_allocated_bytes += size; //NOT SURE ABOUT THIS
        num_meta_data_bytes += sizeof(MallocMetadata);
        return (char*)(block) + (size_t)sizeof(MallocMetadata);
    }
    //not mmap
    MallocMetadata* curr = nullptr;
    for (int i = 0; i < max_order + 1; ++i) {
        curr = free_lists[i];
        while(curr != nullptr){
            if(curr->size >= size){
                if (curr->is_free){
                    //found a right block
                    while(try_to_split(curr, size) > 0);
                    curr->is_free = false;
                    //curr->canary = ourCanary;
                    remove_block_from_free_list(curr);//IS IT NEEDED?
                    num_free_blocks--;
                    num_free_bytes -= curr->size;
                    //num_allocated_blocks++;//should be unchanged because it counts both free and allocated
                    //num_allocated_bytes += curr->size - sizeof(MallocMetadata);//same
                    //num_meta_data_bytes += sizeof(MallocMetadata);//same
                    return (void*)((char*)(curr) + sizeof(MallocMetadata));
                } else{
                    curr = curr->next;
                }
            } else{
                break;//this list is of blocks that are too small
            }
        }
    }
    return nullptr;
}

void* scalloc(size_t num, size_t size){
    void* request = smalloc(num * size);
    if(request == nullptr){
        return nullptr;
    }
    memset(request, 0, size * num);
    return request;
}

void sfree(void* p){
    if(p == nullptr){
        return;
    }
    MallocMetadata* block = (MallocMetadata*)((char*)p - sizeof(MallocMetadata));
    if(block->is_free){
        return;
    }
    check_meta_data(block);
    if(block->size > 128 * 1024 - sizeof(MallocMetadata)){
        //mmap
//        MallocMetadata* curr = mmap_region;
//        while(curr != block){
//            curr = curr->next;
//        }
//        if(curr == mmap_region){
//            mmap_region = mmap_region->next;
//        }
//        else{
//            curr->prev->next = curr->next;
//            if(curr->next != nullptr){
//                curr->next->prev = curr->prev;
//            }
//        }

        //mmap
        num_allocated_blocks--;
        num_allocated_bytes -= block->size;
        num_meta_data_bytes -= sizeof(MallocMetadata);
        munmap(block, block->size + sizeof(MallocMetadata));

        //without ++ to num_free as instructed
        return;
    }

    //not mmap
    block->is_free = true;
    enter_block_into_free_list(block); //IS IT NEEDED? yeah
    num_free_blocks++;
    num_free_bytes += block->size;
    while(try_to_merge(block)){
        MallocMetadata* buddy = find_buddy(block);
        if(buddy < block){
            block = buddy;
        }
    };//straight coding by Nizan the great, aka sir. Makpitz the 3rd of the house of the great coders
    //    bool merged = try_to_merge(block); //diagonal coding by omer
//    if(merged){
//        num_free_blocks++;
//    }

}

void* srealloc(void* oldp, size_t size){
    if(size == 0 || size > 1e8){
        return NULL;
    }
    if(oldp == NULL){
        return smalloc(size);
    }
    auto block = (MallocMetadata*)((char*) oldp - sizeof(MallocMetadata));
    if(block->size < 128 * 1024 - sizeof(MallocMetadata)) {
        /* Not mmap */
        //case a- size is big enough
        check_meta_data(block);
        if (block->size >= size) {
            return oldp;
        }
        //case b- by merging with this block we can do it
        size_t old_size = block->size;
        MallocMetadata* curr = block;
        size_t last_curr_size = curr->size;
        while (true) {
            if (find_order(curr) >= max_order) {
                break;
            }
            if (curr->size >= size) {
                break;
            }
            MallocMetadata *buddy = find_buddy(curr);
            if (find_order(block) == find_order(buddy) && buddy->is_free) {
                if(buddy < curr){
                    curr->size = last_curr_size;
                    curr = buddy;
                    last_curr_size = curr->size;
                }
                curr->size = (curr->size + sizeof(MallocMetadata)) * 2 - sizeof(MallocMetadata);
            } else {
                break;
            }
        }
        if (curr->size >= size) {
            block = curr;
            int required_order = find_order(block);
            block->size = old_size;
            block->is_free = true;
            enter_block_into_free_list(block);

            num_free_blocks++;
            num_free_bytes += block->size;

            while(try_to_merge(block, required_order)){
                MallocMetadata* buddy = find_buddy(block);
                if(buddy < block){
                    block = buddy;
                }
            }
            block->is_free = false;
            remove_block_from_free_list(block);

            num_free_blocks--;
            num_free_bytes -= block->size;//which is the new size

            return (void*)((char*)(block) + sizeof(MallocMetadata));
        }
        curr->size = last_curr_size;
        block->size = old_size;
    }
    //case c- finding another block that is big enough (same in both mmap and regular)
    if (block->size == size){
        return oldp;
    }
    void* newp = smalloc(size);//size will fit mmap or regular matching to oldp as they said
    if(newp == NULL){
        return NULL;
    }
    size_t smaller_size = (block->size < size) ? block->size : size;
    memcpy(newp, oldp, smaller_size);
    sfree(oldp);
    return newp;
}


size_t _num_free_blocks(){
    // Returns the number of allocated blocks in the heap that are currently free.
    return num_free_blocks;
}
size_t _num_free_bytes(){
    //Returns the number of bytes in all allocated blocks in the heap that are currently free,
    //excluding the bytes used by the meta-data structs.
    return num_free_bytes;
}
size_t _num_allocated_blocks(){
    //Returns the overall (free and used) number of allocated blocks in the heap.
    return num_allocated_blocks;
}
size_t _num_allocated_bytes(){
    //Returns the overall number (free and used) of allocated bytes in the heap, excluding
    //the bytes used by the meta-data structs
    return num_allocated_bytes;
}
size_t _num_meta_data_bytes(){
    //Returns the overall number of meta-data bytes currently in the heap.
    return num_meta_data_bytes;
}
size_t _size_meta_data(){
    //Returns the number of bytes of a single meta-data structure in your system.
    return sizeof(MallocMetadata);
}

void print_stats(){
    printf("num_free_blocks: %zu\n", _num_free_blocks());
    printf("num_free_bytes: %zu\n", _num_free_bytes());
    printf("num_allocated_blocks: %zu\n", _num_allocated_blocks());
    printf("num_allocated_bytes: %zu\n", _num_allocated_bytes());
    printf("num_meta_data_bytes: %zu\n", _num_meta_data_bytes());
    printf("size_meta_data: %zu\n", _size_meta_data());
}
#define MAX_ELEMENT_SIZE (128*1024)
//int main(int argc, char** argv){
//    // Allocate and initialize memory with zeros
//    smalloc(0);
//    printf("ran smalloc(0)\n");
//    print_stats();
//    printf("\n");
//
//    void* block1 = smalloc(50);
//    printf("ran smalloc(50)\n");
//    print_stats();
//    printf("\n");
//
//    void* block2 = smalloc(50);
//    printf("ran smalloc(50)\n");
//    print_stats();
//    printf("\n");
//
//    block1 = srealloc(block1, 1000);
//    printf("ran srealloc(1000))\n");
//    print_stats();
//    printf("\n");
//
//    block2 = srealloc(block2, 3000);
//    printf("ran srealloc(3000))\n");
//    print_stats();
//    printf("\n");
//
//
//    sfree(block1);
//    printf("ran free(block1)\n");
//    print_stats();
//    printf("\n");
//
//    sfree(block2);
//    printf("ran free(block2)\n");
//    print_stats();
//    printf("\n");
//
//
//return 0;
//}