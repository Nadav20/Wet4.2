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


struct Block_Node {
    size_t canary; //Where should the canary be placed in the metadata?
    bool is_free;
    size_t size;//actual size of the whole block minus the metadata
    Block_Node* next;
    Block_Node* prev;
};
Block_Node* try_expand_block(Block_Node* block, size_t target_size);
void* allocate_and_copy(void* oldp, Block_Node* old_block, size_t new_size);
void* handle_large_realloc(void* oldp, Block_Node* block, size_t size);
void adjust_block_after_expansion(Block_Node* block, size_t original_size, size_t new_size);
//std::srand(std::time(nullptr));//not working outside a function (nizan)
size_t ourCanary;
const int max_order = 10;
bool first_time = true;
Block_Node* free_lists[max_order + 1];
size_t num_free_blocks = 0;
size_t num_free_bytes = 0;
size_t num_allocated_blocks = 0;
size_t num_allocated_bytes = 0;
size_t num_meta_data_bytes = 0;
Block_Node* mmap_region = nullptr;

Block_Node* find_buddy(Block_Node* block){
    unsigned long address = (unsigned long)block ^ (block->size + sizeof(Block_Node));
    return (Block_Node*)address;
}

int find_order(Block_Node* block){
    return log2((double)(block->size + sizeof(Block_Node))) - 7;
}

void check_meta_data(Block_Node *block){
    if(block->canary != ourCanary){ // if the canary is not equal to our canary
        exit(0xdeadbeef);
    }
}


void remove_block_from_free_list(Block_Node* block){
    check_meta_data(block);
    Block_Node* first = free_lists[find_order(block)], *curr = first;
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

void enter_block_into_free_list(Block_Node* new_block) {
    int order = find_order(new_block);
    new_block->canary = ourCanary;
    Block_Node*& first = free_lists[order];

    // Use binary search to find insertion point
    if (first == nullptr || new_block < first) {
        new_block->next = first;
        new_block->prev = nullptr;
        if (first) first->prev = new_block;
        first = new_block;
        return;
    }

    Block_Node* left = first;
    Block_Node* right = nullptr;
    while (left->next != nullptr) {
        if (new_block < left->next) {
            right = left->next;
            break;
        }
        left = left->next;
    }

    // Insert the new node
    new_block->next = right;
    new_block->prev = left;
    left->next = new_block;
    if (right) right->prev = new_block;
}
bool try_to_merge(Block_Node* block, int required_order = max_order) {
    check_meta_data(block);
    int order = find_order(block);
    if (order >= required_order) {
        return false;
    }

    Block_Node* buddy = find_buddy(block);
    if (buddy->size != block->size || !buddy->is_free) {
        return false;
    }

    check_meta_data(buddy);

    // Determine which block to keep (the one with lower address)
    Block_Node* keep_block = (buddy < block) ? buddy : block;
    //Block_Node* remove_block = (buddy < block) ? block : buddy;

    remove_block_from_free_list(buddy);
    remove_block_from_free_list(block);

    keep_block->size = (keep_block->size + sizeof(Block_Node)) * 2 - sizeof(Block_Node);
    keep_block->is_free = true;

    // Update global counters
    num_free_blocks--;
    num_free_bytes += sizeof(Block_Node);
    num_allocated_blocks--;
    num_allocated_bytes += sizeof(Block_Node);
    num_meta_data_bytes -= sizeof(Block_Node);

    // Re-insert the merged block
    enter_block_into_free_list(keep_block);

    return true;
}


int try_to_split(Block_Node* block_to_split, size_t requested_size) {
    check_meta_data(block_to_split);
    int current_order = find_order(block_to_split);
    size_t total_size = block_to_split->size + sizeof(Block_Node);
    size_t half_size = total_size / 2;

    if (current_order <= 0 || half_size - sizeof(Block_Node) < requested_size) {
        return -1;
    }

    remove_block_from_free_list(block_to_split);

    // Create new buddy block
    Block_Node* new_buddy = reinterpret_cast<Block_Node*>(
            reinterpret_cast<char*>(block_to_split) + half_size
    );

    // Set up both blocks
    block_to_split->size = new_buddy->size = half_size - sizeof(Block_Node);
    block_to_split->is_free = new_buddy->is_free = true;
    block_to_split->canary = new_buddy->canary = ourCanary;

    // Insert both blocks into free list
    enter_block_into_free_list(new_buddy);
    enter_block_into_free_list(block_to_split);

    // Update global counters
    num_free_blocks++;
    num_free_bytes -= sizeof(Block_Node);
    num_allocated_blocks++;
    num_allocated_bytes -= sizeof(Block_Node);
    num_meta_data_bytes += sizeof(Block_Node);

    return current_order - 1;
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

    auto curr = (Block_Node*)((char*)address + bytes_from_allignement);
    for (int i = 0; i < 32; i++) {
        curr->size = 128 * 1024 - sizeof(Block_Node);
        curr->canary = ourCanary;
        curr->is_free = true;
        curr->next = nullptr;
        curr->prev = nullptr;
        enter_block_into_free_list(curr);
        curr = (Block_Node*) ((char*) curr + 128 * 1024);
        num_free_blocks++;
        num_free_bytes += 128 * 1024 - sizeof(Block_Node);
        num_allocated_blocks++;
        num_allocated_bytes += 128 * 1024 - sizeof(Block_Node);
        num_meta_data_bytes += sizeof(Block_Node);
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
    if(size + sizeof(Block_Node) >= 1024 * 128){
        //use mmap
        void* request = mmap(NULL, size + sizeof(Block_Node), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if(request == (void*)(-1)){
            return nullptr;
        }
        Block_Node* block = (Block_Node*)request;
        block->size = size;
        block->canary = ourCanary;
        block->is_free = false;
        //add to mmap_region
        num_allocated_blocks++;
        num_allocated_bytes += size; //NOT SURE ABOUT THIS
        num_meta_data_bytes += sizeof(Block_Node);
        return (char*)(block) + (size_t)sizeof(Block_Node);
    }
    //not mmap
    Block_Node* curr = nullptr;
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
                    //num_allocated_bytes += curr->size - sizeof(Block_Node);//same
                    //num_meta_data_bytes += sizeof(Block_Node);//same
                    return (void*)((char*)(curr) + sizeof(Block_Node));
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
    Block_Node* block = (Block_Node*)((char*)p - sizeof(Block_Node));
    if(block->is_free){
        return;
    }
    check_meta_data(block);
    if(block->size > 128 * 1024 - sizeof(Block_Node)){
        //mmap
//        Block_Node* curr = mmap_region;
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
        num_meta_data_bytes -= sizeof(Block_Node);
        munmap(block, block->size + sizeof(Block_Node));

        //without ++ to num_free as instructed
        return;
    }

    //not mmap
    block->is_free = true;
    enter_block_into_free_list(block); //IS IT NEEDED? yeah
    num_free_blocks++;
    num_free_bytes += block->size;
    while(try_to_merge(block)){
        Block_Node* buddy = find_buddy(block);
        if(buddy < block){
            block = buddy;
        }
    };//straight coding by Nizan the great, aka sir. Makpitz the 3rd of the house of the great coders
    //    bool merged = try_to_merge(block); //diagonal coding by omer
//    if(merged){
//        num_free_blocks++;
//    }

}

void* srealloc(void* oldp, size_t size) {
    if (size == 0 || size > 1e8) {
        return NULL;
    }
    if (oldp == NULL) {
        return smalloc(size);
    }

    auto block = (Block_Node*)((char*)oldp - sizeof(Block_Node));
    check_meta_data(block);

    // Handle large allocations (mmap)
    if (block->size >= 128 * 1024 - sizeof(Block_Node)) {
        return handle_large_realloc(oldp, block, size);
    }

    // Case 1: Current block is big enough
    if (block->size >= size) {
        return oldp;
    }

    // Case 2: Try to expand in-place by merging with buddies
    size_t original_size = block->size;
    Block_Node* expanded_block = try_expand_block(block, size);

    if (expanded_block && expanded_block->size >= size) {
        adjust_block_after_expansion(expanded_block, original_size, size);
        return (void*)((char*)(expanded_block) + sizeof(Block_Node));
    }

    // Case 3: Allocate new block and copy data
    return allocate_and_copy(oldp, block, size);
}

Block_Node* try_expand_block(Block_Node* block, size_t target_size) {
    Block_Node* curr = block;
    size_t current_size = curr->size;

    while (find_order(curr) < max_order && current_size < target_size) {
        Block_Node* buddy = find_buddy(curr);

        // Safety checks
        if (!buddy || buddy->size != curr->size) {
            break;
        }

        check_meta_data(buddy);
        if (!buddy->is_free) {
            break;
        }

        remove_block_from_free_list(buddy);
        current_size = (current_size + sizeof(Block_Node)) * 2 - sizeof(Block_Node);

        if (buddy < curr) {
            curr = buddy;
        }
        curr->size = current_size;
    }
    return curr;
}

void adjust_block_after_expansion(Block_Node* block, size_t original_size, size_t new_size) {
    int required_order = find_order(block);
    block->size = original_size;
    block->is_free = true;
    enter_block_into_free_list(block);

    num_free_blocks++;
    num_free_bytes += block->size;

    while (try_to_merge(block, required_order)) {
        Block_Node* buddy = find_buddy(block);
        if (buddy && buddy < block) {
            block = buddy;
        }
    }
    block->is_free = false;
    remove_block_from_free_list(block);

    num_free_blocks--;
    num_free_bytes -= block->size;
}

void* allocate_and_copy(void* oldp, Block_Node* old_block, size_t new_size) {
    void* newp = smalloc(new_size);
    if (newp == NULL) {
        return NULL;
    }
    size_t copy_size = (old_block->size < new_size) ? old_block->size : new_size;
    memcpy(newp, oldp, copy_size);
    sfree(oldp);
    return newp;
}

void* handle_large_realloc(void* oldp, Block_Node* block, size_t size) {
    if (block->size >= size) {
        return oldp;
    }
    return allocate_and_copy(oldp, block, size);
}


size_t _num_free_blocks(){
    return num_free_blocks;
}
size_t _num_free_bytes(){

    return num_free_bytes;
}
size_t _num_allocated_blocks(){
    return num_allocated_blocks;
}
size_t _num_allocated_bytes(){

    return num_allocated_bytes;
}
size_t _num_meta_data_bytes(){
    return num_meta_data_bytes;
}
size_t _size_meta_data(){
    return sizeof(Block_Node);
}
#define MAX_ELEMENT_SIZE (128*1024)
