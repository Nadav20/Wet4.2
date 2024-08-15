#include <cmath>
#include <sys/mman.h>

#define MAX_ORDER 10
#define Base_of_allocation 128
//List* blocks_list;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
class Block_Node{
private:
    Block_Node* prev;
    Block_Node* next;
    //int num_of_elements_in_block;
    int* first_allocated_byte_in_block;
    size_t real_size_of_block;
    bool block_is_free_flag;
    int* buddy_address;
public:
    Block_Node(int* first_allocated_byte_in_block, size_t size_of_block);
    Block_Node* getPrev();
    Block_Node* getNext();
    int* getFirstAllocatedByteInBlock();
    size_t getSizeOfBlock();
    int* getBuddyAddress();
    //int getNumOfElementsInBlock();
    bool getBlockIsFreeFlag();
    void setPrev(Block_Node* prev);
    void setNext(Block_Node* next);
    void setFirstAllocatedByteInBlock(int* byte);
    void setSizeOfBlock(size_t size);
    void setBlockIsFreeFlag(bool flag);
    void setBuddyAddress(int* buddy_address);
    //void setNumOfElementsInBlock(int num_of_elements_in_block);
};
class List{
private:
    Block_Node* head;
    Block_Node* tail;
    // TODO - add list counter of?
public:
    List();
    List(Block_Node* head);
    ~List();
    Block_Node* getHead();
    Block_Node* getTail();
    void setHead(Block_Node* head);
    void setTail(Block_Node* tail);
    void insertToTail(Block_Node* new_tail);
    Block_Node* removeFromList(int* first_allocated_byte_in_block);
    Block_Node* removeHead();
    void insert_by_first_byte(Block_Node* node);

};Block_Node::Block_Node(int* first_allocated_byte_in_block, size_t size_of_block)
{
    this->next = nullptr;
    this->prev = nullptr;
    //this->num_of_elements_in_block = 0;
    this->block_is_free_flag = false;
    this->first_allocated_byte_in_block = first_allocated_byte_in_block + sizeof(Block_Node);
    this->real_size_of_block = size_of_block + sizeof(Block_Node);
    this->buddy_address = nullptr;
}
int* Block_Node::getFirstAllocatedByteInBlock()
{
    return this->first_allocated_byte_in_block;
}
size_t Block_Node::getSizeOfBlock()
{
    return this->real_size_of_block - sizeof(Block_Node);
}
bool Block_Node::getBlockIsFreeFlag()
{
    return this->block_is_free_flag;
}
Block_Node* Block_Node::getPrev()
{
    return this->prev;
}
Block_Node* Block_Node::getNext()
{
    return this->next;
}
/*int Block_Node::getNumOfElementsInBlock()
{
    return this->num_of_elements_in_block;
}*/
int* Block_Node::getBuddyAddress()
{
    return this->buddy_address;
}
void Block_Node::setPrev(Block_Node* prev)
{
    this->prev = prev;
    prev->next = this;
}
void Block_Node::setNext(Block_Node* next)
{
    this->next = next;
    next->prev = this;
}
void Block_Node::setFirstAllocatedByteInBlock(int* byte)
{
    this->first_allocated_byte_in_block = byte + sizeof(Block_Node);
}
void Block_Node::setSizeOfBlock(size_t size)
{
    this->real_size_of_block = size + sizeof(Block_Node);
}
void Block_Node::setBlockIsFreeFlag(bool flag)
{
    this->block_is_free_flag = flag;
}
/*void Block_Node::setNumOfElementsInBlock(int num_of_elements_in_block)
{
    this->num_of_elements_in_block = num_of_elements_in_block;
}*/

void Block_Node::setBuddyAddress(int* buddy_address)
{
    this->buddy_address = buddy_address;
}
List::List()
{
    this->head = nullptr;
    this->tail = nullptr;
}
List::List(Block_Node* head)
{
    this->head = head;
    this->tail = this->head;
}
List::~List()
{
    Block_Node* current = this->getHead();
    Block_Node* next = nullptr;
    while (current != nullptr)
    {
        next = current->getNext();
        delete current->getFirstAllocatedByteInBlock();
        delete current;
        current = next;
    }
}
Block_Node* List::getHead()
{
    return this->head;
}
Block_Node* List::getTail()
{
    return this->tail;
}
void List::setHead(Block_Node* head)
{
    this->head = head;
}
void List::setTail(Block_Node* tail)
{
    this->tail = tail;
}
void List::insertToTail(Block_Node* new_tail)
{
    if (this->head == nullptr) // the list is empty
    {
        this->setHead(new_tail);
        this->tail = this->head;
    }
    else
    {
        this->tail->setNext(new_tail);
        this->setTail(new_tail);
    }
}
Block_Node* List::removeFromList(int* first_allocated_byte_in_block)
{
    Block_Node* current = this->head;
    if (current->getFirstAllocatedByteInBlock() == first_allocated_byte_in_block) // we want to remove the head of the list
    {
        return removeHead();
    }
    while (current != nullptr)
    {
        if (current->getFirstAllocatedByteInBlock() == first_allocated_byte_in_block)
        {
            current->getPrev()->setNext(current->getNext());
            if (current->getNext() != nullptr)
            {
                current->getNext()->setPrev(current->getPrev());
            }
            else
            {
                this->setTail(current->getPrev());
            }
            current->setPrev(nullptr); // not sure if needed
            current->setNext(nullptr); // not sure if needed
            return current;
        }
        current = current->getNext();
    }
    return nullptr;
}
Block_Node* List::removeHead()
{
    if (this->head == nullptr)
    {
        return nullptr;
    }
    Block_Node* old_head = new Block_Node(this->head->getFirstAllocatedByteInBlock(),this->head->getSizeOfBlock());
    head = head->getNext();
    delete head->getPrev();
    head->setPrev(nullptr);
    return old_head;
}

void List::insert_by_first_byte(Block_Node* node)
{
    if (this->head == nullptr)
    {
        this->setHead(node);
        this->setTail(node);
        return;
    }
    Block_Node* current_node = this->head;
    while (current_node != nullptr)
    {
        if (node->getFirstAllocatedByteInBlock() < current_node->getFirstAllocatedByteInBlock())
        {
            if (current_node->getFirstAllocatedByteInBlock() == this->head->getFirstAllocatedByteInBlock()) // current_node is head
            {
                this->setHead(node);
                current_node->setPrev(node);
                node->setNext(current_node);
            }
            else
            {
                current_node->getPrev()->setNext(node);
                node->setPrev(current_node->getPrev());
                node->setNext(current_node);
                current_node->setPrev(node);
            }
            return;
        }
        current_node = current_node->getNext();
    }
    this->insertToTail(node);
}

static int number_of_mallocs_called = 0;

List *free_blocks_list_arr[MAX_ORDER+1];
List *taken_blocks_list_arr[MAX_ORDER+1];
// maybe add huge_blocks list (blocks that are bigger than 128kB)
int* our_previous_program_break = 0;

Block_Node* split_block(Block_Node* block, int index, size_t size){
    if(index == 0){
        return nullptr; //cant split
    }
    size_t current_size = block->getSizeOfBlock(); // size of block not indlude MD
    size_t real_size = current_size + sizeof(Block_Node);
    int* current_byte = block->getFirstAllocatedByteInBlock();
    int* real_byte = current_byte - sizeof(Block_Node);
    Block_Node* new_block1 = nullptr;
    while(real_size >= 2* (size + sizeof(Block_Node)) && index >= 1){
        block = free_blocks_list_arr[index]->removeFromList(current_byte);
        delete(block);
        new_block1 = new Block_Node(real_byte, real_size/2 - sizeof(Block_Node));
        Block_Node* new_block2 = new Block_Node(real_byte+real_size/2, real_size/2 - sizeof(Block_Node));
        new_block1->setBuddyAddress(new_block2->getFirstAllocatedByteInBlock());
        new_block2->setBuddyAddress(new_block1->getFirstAllocatedByteInBlock());
        free_blocks_list_arr[index-1]->insert_by_first_byte(new_block1);
        free_blocks_list_arr[index-1]->insert_by_first_byte(new_block2);
        index--;
        real_size = real_size / 2;
    }
    return new_block1;
}
Block_Node* merge_the_blocks(Block_Node* left_block, Block_Node* right_block, int index);

Block_Node* merge_block(Block_Node* block, int index){ //just want to free the block
    size_t current_size = block->getSizeOfBlock();
    int* current_first_byte = block->getFirstAllocatedByteInBlock();
    while(current_first_byte +current_size + sizeof(Block_Node) == block->getNext()->getFirstAllocatedByteInBlock())
    {
        if (block->getBuddyAddress() != nullptr){
            if(block->getNext()->getBuddyAddress() != nullptr){
                if(block->getBuddyAddress() == block->getNext()->getBuddyAddress()){
                    block = merge_the_blocks(block, block->getNext(), index);
                    if(block == nullptr)
                        break;
                    index++;
                    current_size = block->getSizeOfBlock();
                    current_first_byte = block->getFirstAllocatedByteInBlock();
                }
            }
        }
        if(index >= 10){
            break;
        }
    }

    while(current_first_byte == block->getPrev()->getFirstAllocatedByteInBlock()+ block->getPrev()->getSizeOfBlock() + sizeof(Block_Node) ){
        if (block->getBuddyAddress() != nullptr){
            if(block->getPrev()->getBuddyAddress() != nullptr){
                if(block->getBuddyAddress() == block->getPrev()->getBuddyAddress()){
                    block = merge_the_blocks(block->getPrev(), block, index);
                    if(block == nullptr)
                        break;
                    index++;
                    current_first_byte = block->getFirstAllocatedByteInBlock();
                }
            }
        }
        if(index >= 10){
            break;
        }
    }
    return block;
}

Block_Node* merge_the_blocks(Block_Node* left_block, Block_Node* right_block, int index){
    if(index>= MAX_ORDER){
        return nullptr;
    }
    int new_real_size = ( left_block->getSizeOfBlock() + sizeof(Block_Node) ) * 2;
    int* new_real_first_byte = left_block->getFirstAllocatedByteInBlock() - sizeof(Block_Node);
    free_blocks_list_arr[index]->removeFromList(left_block->getFirstAllocatedByteInBlock());
    free_blocks_list_arr[index]->removeFromList(right_block->getFirstAllocatedByteInBlock());
    Block_Node* merged_block = new Block_Node(new_real_first_byte, new_real_size-sizeof(Block_Node));
    free_blocks_list_arr[index+1]->insert_by_first_byte(merged_block);
    return merged_block;
}


void* smalloc(size_t size)
{
    Block_Node* new_block;
    //int* first_byte_in_the_allocated_block;
    size_t actual_size;
    if (number_of_mallocs_called == 0) //allocate the initial 32 blocks of 128kb
    {
        actual_size = Base_of_allocation*pow(2,10) + sizeof(Block_Node);
        for (int i = 0;i < 32;i++)
        {
            int* new_block_node_address = (int*)mmap(nullptr,actual_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (new_block_node_address == (int*)-1)
            {
                return NULL;
            }
            new_block = (Block_Node*)new_block_node_address;
            new_block->setFirstAllocatedByteInBlock(new_block_node_address);
            new_block->setSizeOfBlock(Base_of_allocation*pow(2,10));
            *our_previous_program_break += actual_size;
            free_blocks_list_arr[MAX_ORDER]->insertToTail(new_block);
        }
    }
    number_of_mallocs_called++;
    if (size == 0)
    {
        return nullptr;
    }
    for (int i = (int)log2(size / Base_of_allocation);i<MAX_ORDER;i++) // looks for a free block greater or equal than size remove from free arr and insert to taken arr
    {
        if (free_blocks_list_arr[i]->getHead() != nullptr)
        {
            Block_Node* block_after_split = split_block(free_blocks_list_arr[i]->getHead(), i, size);
            if (block_after_split == nullptr)
            {
                return NULL;
            }

            Block_Node* current_block_node = free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation)]->getHead();
            while (current_block_node != nullptr)
            {
                    if (current_block_node->getFirstAllocatedByteInBlock() ==block_after_split->getFirstAllocatedByteInBlock())
                    {
                        free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation)]->removeFromList(current_block_node->getFirstAllocatedByteInBlock());
                        taken_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation)]->insertToTail(current_block_node);
                        *our_previous_program_break += actual_size;
                        return current_block_node->getFirstAllocatedByteInBlock();
                    }       
                    current_block_node = current_block_node->getNext();
            }

            if ((int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) > 0)
            {
                current_block_node = free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) - 1]->getHead();
                while (current_block_node)
                {
                    if (current_block_node->getFirstAllocatedByteInBlock() ==block_after_split->getFirstAllocatedByteInBlock())
                    {
                        free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) - 1]->removeFromList(current_block_node->getFirstAllocatedByteInBlock());
                        taken_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) - 1]->insertToTail(current_block_node);
                        *our_previous_program_break += actual_size;
                        return current_block_node->getFirstAllocatedByteInBlock();
                    }       
                    current_block_node = current_block_node->getNext();
                    
                }
            }
            if ((int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) < MAX_ORDER)
            {
                current_block_node = free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) + 1]->getHead();
                while (current_block_node)
                {
                    if (current_block_node->getFirstAllocatedByteInBlock() ==block_after_split->getFirstAllocatedByteInBlock())
                    {
                        free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) + 1]->removeFromList(current_block_node->getFirstAllocatedByteInBlock());
                        taken_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) + 1]->insertToTail(current_block_node);
                        *our_previous_program_break += actual_size;
                        return current_block_node->getFirstAllocatedByteInBlock();
                    }       
                    current_block_node = current_block_node->getNext();
                }
            }
            return block_after_split->getFirstAllocatedByteInBlock();
        }
    }

    //if we reached this line, then there was no free block with the right size found
    actual_size = size + sizeof(Block_Node);
    new_block = nullptr;
    if (size >= Base_of_allocation*pow(2,10)) // TODO: maybe actual_size instead of size?
    {
        // mmap of size 'size' in order to enlarge the heap and then put the block of size 'size' in the heap
        int* new_block_node_address = (int*)mmap(nullptr,actual_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (new_block_node_address == (int*)-1)
        {
            return NULL;
        }
        new_block = (Block_Node*)new_block_node_address;
        new_block->setFirstAllocatedByteInBlock(new_block_node_address);
        new_block->setSizeOfBlock(size);
        *our_previous_program_break += actual_size;
        taken_blocks_list_arr[MAX_ORDER]->insertToTail(new_block);
    }
    else{
        actual_size = Base_of_allocation*pow(2,10) + sizeof(Block_Node);
        int* new_block_node_address = (int*)mmap(nullptr,actual_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (new_block_node_address == (int*)-1)
        {
            return NULL;
        }
        new_block = (Block_Node*)new_block_node_address;
        new_block->setFirstAllocatedByteInBlock(new_block_node_address);
        new_block->setSizeOfBlock(Base_of_allocation*pow(2,10));
        *our_previous_program_break += actual_size;
        free_blocks_list_arr[MAX_ORDER]->insert_by_first_byte(new_block);
        Block_Node* block_after_split = split_block(new_block,MAX_ORDER,size);
        if (block_after_split == nullptr)
        {
            return NULL;
        }

        Block_Node* current_block_node = free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation)]->getHead();
        while (current_block_node)
        {
                if (current_block_node->getFirstAllocatedByteInBlock() ==block_after_split->getFirstAllocatedByteInBlock())
                {
                     free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation)]->removeFromList(current_block_node->getFirstAllocatedByteInBlock());
                     taken_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation)]->insertToTail(current_block_node);
                     *our_previous_program_break += actual_size;
                     return current_block_node->getFirstAllocatedByteInBlock();
                }       
                current_block_node = current_block_node->getNext();
        }

        if ((int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) > 0)
        {
            current_block_node = free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) - 1]->getHead();
            while (current_block_node)
            {
                if (current_block_node->getFirstAllocatedByteInBlock() ==block_after_split->getFirstAllocatedByteInBlock())
                {
                    free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) - 1]->removeFromList(current_block_node->getFirstAllocatedByteInBlock());
                     taken_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) - 1]->insertToTail(current_block_node);
                     *our_previous_program_break += actual_size;
                     return current_block_node->getFirstAllocatedByteInBlock();
                }       
                current_block_node = current_block_node->getNext();
                
            }
        }
        if ((int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) < MAX_ORDER)
        {
            current_block_node = free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) + 1]->getHead();
            while (current_block_node)
            {
                if (current_block_node->getFirstAllocatedByteInBlock() ==block_after_split->getFirstAllocatedByteInBlock())
                {
                    free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) + 1]->removeFromList(current_block_node->getFirstAllocatedByteInBlock());
                     taken_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) + 1]->insertToTail(current_block_node);
                     *our_previous_program_break += actual_size;
                     return current_block_node->getFirstAllocatedByteInBlock();
                }       
                current_block_node = current_block_node->getNext();
                
            }
        }
        return block_after_split->getFirstAllocatedByteInBlock();
        //taken_blocks_list_arr[(int)log2((size/Base_of_allocation))]->insertToTail(new_block);
    }
    return nullptr;
}

void* scalloc(size_t num, size_t size)
{
    Block_Node* new_block;
    //int* first_byte_in_the_allocated_block;
    int actual_size;
    if (number_of_mallocs_called == 0) //allocate the initial 32 blocks of 128kb
    {
        actual_size = Base_of_allocation*pow(2,10) + sizeof(Block_Node);
        for (int i = 0;i < 32;i++)
        {
            int* new_block_node_address = (int*)mmap(nullptr,actual_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (new_block_node_address == (int*)-1)
            {
                return NULL;
            }
            new_block = (Block_Node*)new_block_node_address;
            new_block->setFirstAllocatedByteInBlock(new_block_node_address);
            new_block->setSizeOfBlock(Base_of_allocation*pow(2,10));
            *our_previous_program_break += actual_size;
            free_blocks_list_arr[MAX_ORDER]->insertToTail(new_block);
        }
    }
    number_of_mallocs_called++;

    if (size == 0 || num == 0)
    {
        return NULL;
    }
    if (size * num > pow(10,8)) // not sure if needed here
    {
        return NULL;
    }

    for (int i = (int)log2((size * num) / Base_of_allocation);i<MAX_ORDER;i++) // looks for a free block greater or equal than size remove from free arr and insert to taken arr
    {
        if (free_blocks_list_arr[i]->getHead() != nullptr)
        {
            Block_Node* block_after_split = split_block(new_block,MAX_ORDER,size);
            if (block_after_split == nullptr)
            {
                return NULL;
            }

            Block_Node* current_block_node = free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation)]->getHead();
            while (current_block_node != nullptr)
            {
                    if (current_block_node->getFirstAllocatedByteInBlock() ==block_after_split->getFirstAllocatedByteInBlock())
                    {
                        free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation)]->removeFromList(current_block_node->getFirstAllocatedByteInBlock());
                        taken_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation)]->insertToTail(current_block_node);
                        *our_previous_program_break += actual_size;
                        memset(current_block_node->getFirstAllocatedByteInBlock(), 0, size*num);
                        return current_block_node->getFirstAllocatedByteInBlock();
                    }       
                    current_block_node = current_block_node->getNext();
            }

            if ((int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) > 0)
            {
                current_block_node = free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) - 1]->getHead();
                while (current_block_node)
                {
                    if (current_block_node->getFirstAllocatedByteInBlock() ==block_after_split->getFirstAllocatedByteInBlock())
                    {
                        free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) - 1]->removeFromList(current_block_node->getFirstAllocatedByteInBlock());
                        taken_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) - 1]->insertToTail(current_block_node);
                        *our_previous_program_break += actual_size;
                        memset(current_block_node->getFirstAllocatedByteInBlock(), 0, size*num);
                        return current_block_node->getFirstAllocatedByteInBlock();
                    }       
                    current_block_node = current_block_node->getNext();
                    
                }
            }
            if ((int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) < MAX_ORDER)
            {
                current_block_node = free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) + 1]->getHead();
                while (current_block_node)
                {
                    if (current_block_node->getFirstAllocatedByteInBlock() ==block_after_split->getFirstAllocatedByteInBlock())
                    {
                        free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) + 1]->removeFromList(current_block_node->getFirstAllocatedByteInBlock());
                        taken_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) + 1]->insertToTail(current_block_node);
                        *our_previous_program_break += actual_size;
                        memset(current_block_node->getFirstAllocatedByteInBlock(), 0, size*num);
                        return current_block_node->getFirstAllocatedByteInBlock();
                    }       
                    current_block_node = current_block_node->getNext();
                }
            }
            return block_after_split->getFirstAllocatedByteInBlock();
        }
    }

    //if we reached this line, then there was no free block found
    actual_size = size * num + sizeof(Block_Node);
    new_block = nullptr;
    if (size * num >= Base_of_allocation*pow(2,10)) // TODO maybe (size * num) + sizeof(Block_Node)?
    {
        // mmap of size 'size' in order to enlarge the heap and then put the block of size 'size' in the heap
        int* new_block_node_address = (int*)mmap(nullptr,actual_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (new_block_node_address == (int*)-1)
        {
            return NULL;
        }
        new_block = (Block_Node*)new_block_node_address;
        new_block->setFirstAllocatedByteInBlock(new_block_node_address);
        new_block->setSizeOfBlock(size);
        *our_previous_program_break += actual_size;
        memset(new_block->getFirstAllocatedByteInBlock(), 0, size*num);
        taken_blocks_list_arr[MAX_ORDER]->insertToTail(new_block);
    }
    else
    {
        actual_size = Base_of_allocation*pow(2,10) + sizeof(Block_Node);
        int* new_block_node_address = (int*)mmap(nullptr,actual_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (new_block_node_address == (int*)-1)
        {
            return NULL;
        }
        new_block = (Block_Node*)new_block_node_address;
        new_block->setFirstAllocatedByteInBlock(new_block_node_address);
        new_block->setSizeOfBlock(Base_of_allocation*pow(2,10));
        *our_previous_program_break += actual_size;
        free_blocks_list_arr[MAX_ORDER]->insert_by_first_byte(new_block);
        Block_Node* block_after_split = split_block(new_block,MAX_ORDER,size);
        if (block_after_split == nullptr)
        {
            return NULL;
        }

        Block_Node* current_block_node = free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation)]->getHead();
        while (current_block_node != nullptr)
        {
                if (current_block_node->getFirstAllocatedByteInBlock() ==block_after_split->getFirstAllocatedByteInBlock())
                {
                     free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation)]->removeFromList(current_block_node->getFirstAllocatedByteInBlock());
                     taken_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation)]->insertToTail(current_block_node);
                     *our_previous_program_break += actual_size;
                     memset(current_block_node->getFirstAllocatedByteInBlock(), 0, size*num);
                     return current_block_node->getFirstAllocatedByteInBlock();
                }       
                current_block_node = current_block_node->getNext();
        }

        if ((int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) > 0)
        {
            current_block_node = free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) - 1]->getHead();
            while (current_block_node)
            {
                if (current_block_node->getFirstAllocatedByteInBlock() ==block_after_split->getFirstAllocatedByteInBlock())
                {
                    free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) - 1]->removeFromList(current_block_node->getFirstAllocatedByteInBlock());
                     taken_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) - 1]->insertToTail(current_block_node);
                     *our_previous_program_break += actual_size;
                     memset(current_block_node->getFirstAllocatedByteInBlock(), 0, size*num);
                     return current_block_node->getFirstAllocatedByteInBlock();
                }       
                current_block_node = current_block_node->getNext();
                
            }
        }
        if ((int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) < MAX_ORDER)
        {
            current_block_node = free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) + 1]->getHead();
            while (current_block_node)
            {
                if (current_block_node->getFirstAllocatedByteInBlock() ==block_after_split->getFirstAllocatedByteInBlock())
                {
                    free_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) + 1]->removeFromList(current_block_node->getFirstAllocatedByteInBlock());
                     taken_blocks_list_arr[(int)log2(block_after_split->getSizeOfBlock() / Base_of_allocation) + 1]->insertToTail(current_block_node);
                     *our_previous_program_break += actual_size;
                     memset(current_block_node->getFirstAllocatedByteInBlock(), 0, size*num);
                     return current_block_node->getFirstAllocatedByteInBlock();
                }       
                current_block_node = current_block_node->getNext();
            }
        }
        return block_after_split->getFirstAllocatedByteInBlock();
        //taken_blocks_list_arr[(int)log2((size/Base_of_allocation))]->insertToTail(new_block);
    }
    return nullptr;
}
void sfree(void* p)
{
    if (p == NULL)
    {
        return;
    }
    for (int i = 0;i < MAX_ORDER;i++)
    {
        Block_Node* current_taken_node = taken_blocks_list_arr[i]->getHead();
        while (current_taken_node != nullptr)
        {
            if (current_taken_node->getFirstAllocatedByteInBlock() == (int*)p)
            {
                Block_Node* node_to_be_freed = merge_block(current_taken_node, i);
                node_to_be_freed = free_blocks_list_arr[i]->removeFromList((int*)p);
                free_blocks_list_arr[i]->insert_by_first_byte(node_to_be_freed);
                return;
            }
            current_taken_node = current_taken_node->getNext();
        }
    }
    Block_Node* current_taken_node = taken_blocks_list_arr[MAX_ORDER]->getHead();
    while (current_taken_node != nullptr)
    {
        if (current_taken_node->getFirstAllocatedByteInBlock() == (int*)p)
        {
            int return_value_from_munmap = munmap(p,current_taken_node->getSizeOfBlock());
            if (return_value_from_munmap == -1)
            {
                return;
            }
            Block_Node* node_to_be_freed = taken_blocks_list_arr[MAX_ORDER]->removeFromList((int*)p);
            free_blocks_list_arr[MAX_ORDER]->insert_by_first_byte(node_to_be_freed);
            return;
        }
        current_taken_node = current_taken_node->getNext();
    }
}

void* srealloc(void* oldp, size_t size)
{
    Block_Node* new_block_node = nullptr;
    int actual_size;
    int* first_byte_in_the_allocated_block;
    if (size == 0)
    {
        return NULL;
    }
    if (size >= pow(10, 8))
    {
        return NULL;
    }
    if (oldp == nullptr) // NULL instead of nullptr?
    {
        actual_size = size + sizeof(Block_Node);// maybe should be with "+ sizeof(Block_Node*)"?
        if (actual_size >= Base_of_allocation * pow(2,10))
        {
            int* new_block_node_address = (int*)mmap(nullptr,actual_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (new_block_node_address == (int*)-1)
            {
                return NULL;
            }
            new_block_node = (Block_Node*)new_block_node_address;
            new_block_node->setFirstAllocatedByteInBlock(new_block_node_address);
            new_block_node->setSizeOfBlock(actual_size);
            taken_blocks_list_arr[MAX_ORDER]->insertToTail(new_block_node);
        }
        else
        {
            int* first_byte_in_the_allocated_block = (int*)sbrk(actual_size);
            if (*first_byte_in_the_allocated_block == -1)
            {
                return NULL;
            }
            new_block_node = new Block_Node(first_byte_in_the_allocated_block, size);
            if (new_block_node == nullptr)
            {
                return NULL;
            }
            taken_blocks_list_arr[(int)log2(size / Base_of_allocation)]->insertToTail(new_block_node);
        }
        *our_previous_program_break += actual_size;
        return new_block_node->getFirstAllocatedByteInBlock();
    }
    
    Block_Node* current_block_node = taken_blocks_list_arr[(int)log2(size / Base_of_allocation)]->getHead();
    while (current_block_node != nullptr)
    {
        if (current_block_node->getFirstAllocatedByteInBlock() == (int*)oldp)
        {
            if (size <= current_block_node->getSizeOfBlock())
            {
                // reuse the same block
                return current_block_node->getFirstAllocatedByteInBlock();
            }
        }
        current_block_node = current_block_node->getNext();
    }

    for (int i = (int)log2(size / Base_of_allocation);i<MAX_ORDER + 1;i++)
    {
        if (free_blocks_list_arr[i]->getHead() != nullptr)
        {
            Block_Node* free_block = free_blocks_list_arr[i]->removeHead();
            free_block->setFirstAllocatedByteInBlock((int*)oldp);
            taken_blocks_list_arr[i]->insertToTail(free_block);
            free(oldp);
            return free_block->getFirstAllocatedByteInBlock();
        }   
    }

    //if we reached this line, then there was no free block with the right size found
    actual_size = size + sizeof(Block_Node);
    if (size > Base_of_allocation*pow(2,10)) 
    {
        // mmap of size 'size' in order to enlarge the heap and then put the block of size 'size' in the heap
        int* new_block_node_address = (int*)mmap(nullptr,actual_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (new_block_node_address == (int*)-1)
        {
            return NULL;
        }
        new_block_node = (Block_Node*)new_block_node_address;
        new_block_node->setFirstAllocatedByteInBlock((int*)oldp);
        new_block_node->setSizeOfBlock(size);
        taken_blocks_list_arr[MAX_ORDER]->insertToTail(new_block_node);
    }
    else{
        // same as with mmap but with sbrk
        first_byte_in_the_allocated_block = (int*)sbrk(actual_size);
        if (*first_byte_in_the_allocated_block == -1)
        {
            return NULL;
        }
        new_block_node = new Block_Node(first_byte_in_the_allocated_block,size);
        if (new_block_node == nullptr)
        {
            return NULL;
        }
        new_block_node->setFirstAllocatedByteInBlock((int*)oldp);
        taken_blocks_list_arr[(int)log2((size/Base_of_allocation))]->insertToTail(new_block_node);
    }
    *our_previous_program_break += actual_size;
    return new_block_node->getFirstAllocatedByteInBlock();
}

///////////////////////////////////// NADAV 5-9 Funcs //////////////////////////////////////////////


size_t _num_free_blocks()
{
    int counter_free_blocks = 0;
    Block_Node* current_block_node;
    for (int i = 0;i<MAX_ORDER + 1;i++)
    {
        current_block_node = free_blocks_list_arr[i]->getHead();
        while (current_block_node != nullptr)
        {
            counter_free_blocks++;
            current_block_node = current_block_node->getNext();
        }
    }
    return counter_free_blocks;
}


size_t _num_free_bytes()
{
    int counter_free_bytes = 0;
    Block_Node* current_block_node;
    for (int i = 0;i<MAX_ORDER + 1;i++)
    {
        current_block_node = free_blocks_list_arr[i]->getHead();
        while (current_block_node != nullptr)
        {
            counter_free_bytes++;
            current_block_node = current_block_node->getNext();
        }
    }
    return counter_free_bytes;
}

size_t _num_allocated_blocks(){
    int _num_taken_blocks = 0;
    Block_Node* current_block_node;
    for (int i = 0;i<MAX_ORDER + 1;i++)
    {
        current_block_node = taken_blocks_list_arr[i]->getHead();
        while (current_block_node != nullptr)
        {
            _num_taken_blocks++;
            current_block_node = current_block_node->getNext();
        }
    }
    return _num_taken_blocks + _num_free_blocks();
}



size_t _num_allocated_bytes(){
    int counter_taken_bytes = 0;
    Block_Node* current_block_node;
    for (int i = 0;i<MAX_ORDER + 1;i++)
    {
        current_block_node = taken_blocks_list_arr[i]->getHead();
        while (current_block_node != nullptr)
        {
            counter_taken_bytes++;
            current_block_node = current_block_node->getNext();
        } 
    }
    return counter_taken_bytes + _num_free_bytes();
}

////////// not sure about the last two functions
size_t _num_meta_data_bytes(){

    return _num_allocated_blocks() * sizeof(Block_Node);
}

size_t _size_meta_data(){
    return sizeof(Block_Node);
}

//challenge 0

