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


List* blocks_list;
void* smalloc(size_t size)
{
    if (size == 0)
    {
        return NULL;
    }
    if (size > pow(10,8))
    {
        return NULL;
    }
    int* first_byte_in_the_allocated_block;
    Block_Node* current_block_node = blocks_list->getHead();
    while (current_block_node != nullptr)
    {
        if (current_block_node->getSizeOfBlock() >= size && current_block_node->getBlockIsFreeFlag() == true)
        {
            first_byte_in_the_allocated_block = current_block_node->getFirstAllocatedByteInBlock();
            current_block_node->setBlockIsFreeFlag(false);
            return first_byte_in_the_allocated_block;
        }
        current_block_node = current_block_node->getNext();
    }
    int actual_size = size + sizeof(Block_Node);// maybe should be with "+ sizeof(Block_Node*)"?
    first_byte_in_the_allocated_block = (int*)sbrk(actual_size);
    if (*first_byte_in_the_allocated_block == -1)
    {
        return NULL;
    }
    Block_Node* new_block_node = new Block_Node(first_byte_in_the_allocated_block,size);
    if (new_block_node == nullptr)
    {
        return NULL;
    }
    blocks_list->insertToTail(new_block_node);
    return first_byte_in_the_allocated_block;
}
void* scalloc(size_t num, size_t size)
{
    if (size == 0 || num == 0)
    {
        return NULL;
    }
    if (size * num > pow(10,8))
    {
        return NULL;
    }
    int* first_byte_in_the_allocated_block;
    Block_Node* current_block_node = blocks_list->getHead();
    while (current_block_node != nullptr)
    {
        if (current_block_node->getSizeOfBlock() >= size * num && current_block_node->getBlockIsFreeFlag() == true)
        {
            first_byte_in_the_allocated_block = current_block_node->getFirstAllocatedByteInBlock();
            current_block_node->setBlockIsFreeFlag(false);
            memset(first_byte_in_the_allocated_block, 0, size*num);
            return first_byte_in_the_allocated_block;
        }
        current_block_node = current_block_node->getNext();
    }
    int actual_size = (size * num) + sizeof(Block_Node);// maybe should be with "+ sizeof(Block_Node*)"?
    first_byte_in_the_allocated_block = (int*)sbrk(actual_size);
    if (*first_byte_in_the_allocated_block == -1)
    {
        return NULL;
    }
    Block_Node* new_block_node = new Block_Node(first_byte_in_the_allocated_block, size * num);
    if (new_block_node == nullptr)
    {
        return NULL;
    }
    if (blocks_list->getHead() == nullptr) // blocks_list is empty
    {
        
        blocks_list->setHead(new_block_node);
        blocks_list->setTail(new_block_node);
    }
    else
    {
        blocks_list->insertToTail(new_block_node);
    }
    memset(first_byte_in_the_allocated_block, 0, size*num);
    return first_byte_in_the_allocated_block;
}
void sfree(void* p)
{
    if (p == NULL)
    {
        return;
    }
    Block_Node* current_block_node = blocks_list->getHead();
    while (current_block_node != nullptr)
    {
        if (current_block_node->getFirstAllocatedByteInBlock() == (int*)p)
        {
            if (current_block_node->getBlockIsFreeFlag() == false)
            {
                current_block_node->setBlockIsFreeFlag(true);
            }
            return;
        }
        current_block_node = current_block_node->getNext();
    } 
}
void* srealloc(void* oldp, size_t size)
{
    int actual_size;
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
        int* first_byte_in_the_allocated_block = (int*)sbrk(actual_size);
        if (*first_byte_in_the_allocated_block == -1)
        {
            return NULL;
        }
        Block_Node* new_block_node = new Block_Node(first_byte_in_the_allocated_block, size);
        if (new_block_node == nullptr)
        {
            return NULL;
        }
        blocks_list->insertToTail(new_block_node);
        return first_byte_in_the_allocated_block;
    }
    
    Block_Node* current_block_node = blocks_list->getHead();
    while (current_block_node != nullptr)
    {
        if (current_block_node->getFirstAllocatedByteInBlock() == (int*)oldp)
        {
            if (size <= current_block_node->getSizeOfBlock())
            {
                // reuse the same block, e.g. don't do anything
                return current_block_node->getFirstAllocatedByteInBlock();
            }
        }
        current_block_node = current_block_node->getNext();
    }

    current_block_node = blocks_list->getHead();
    while (current_block_node != nullptr)
    {
        if (current_block_node->getSizeOfBlock() >= size) 
        {
            if (current_block_node->getBlockIsFreeFlag() == true)
            {
                current_block_node->setFirstAllocatedByteInBlock((int*)oldp);
                current_block_node->setBlockIsFreeFlag(false);
                free(oldp);
                return current_block_node->getFirstAllocatedByteInBlock();
            }
        }
        current_block_node = current_block_node->getNext();
    }

    actual_size = size + sizeof(Block_Node);// maybe should be with "+ sizeof(Block_Node*)"?
    int* first_byte_in_the_allocated_block = (int*)sbrk(actual_size);
    if (*first_byte_in_the_allocated_block == -1)
    {
        return NULL;
    }
    Block_Node* new_block_node = new Block_Node(first_byte_in_the_allocated_block,size);
    if (new_block_node == nullptr)
    {
        return NULL;
    }
    new_block_node->setFirstAllocatedByteInBlock((int*)oldp);
    blocks_list->insertToTail(new_block_node);
    free(oldp);
    return blocks_list->getTail()->getFirstAllocatedByteInBlock();
}

///////////////////////////////////// NADAV 5-9 Funcs //////////////////////////////////////////////


size_t _num_free_blocks(){
    int counter_free_blocks = 0;
    Block_Node* current_block_node = blocks_list->getHead();
    while (current_block_node != nullptr)
    {
        if (current_block_node->getBlockIsFreeFlag() == true)
        {
            counter_free_blocks++;
        }
        current_block_node = current_block_node->getNext();
    }
    return counter_free_blocks;
}


size_t _num_free_bytes(){
    int counter_free_bytes = 0;
    Block_Node* current_block_node = blocks_list->getHead();
    while (current_block_node != nullptr)
    {
        if (current_block_node->getBlockIsFreeFlag())
        {
            counter_free_bytes += current_block_node->getSizeOfBlock();
        }
        current_block_node = current_block_node->getNext();
    }
    return counter_free_bytes;
}

size_t _num_allocated_blocks(){
    int _num_allocated_blocks = 0;
    Block_Node* current_block_node = blocks_list->getHead();
    while (current_block_node != nullptr)
    {
        if (current_block_node->getBlockIsFreeFlag())
        {
            _num_allocated_blocks++;
        }
        current_block_node = current_block_node->getNext();
    }
    return _num_allocated_blocks;
}



size_t _num_allocated_bytes(){
    int counter_allocated_bytes = 0;
    Block_Node* current_block_node = blocks_list->getHead();
    while (current_block_node != nullptr)
    {
        counter_allocated_bytes += current_block_node->getSizeOfBlock();
        current_block_node = current_block_node->getNext();
    }
    return counter_allocated_bytes;
}

////////// not sure about the last two functions
size_t _num_meta_data_bytes(){
    return _num_allocated_blocks() * sizeof(Block_Node);
}

size_t _size_meta_data(){
    return sizeof(Block_Node);
}