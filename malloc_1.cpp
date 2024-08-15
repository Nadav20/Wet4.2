#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
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
    int* previous_program_break = (int*)sbrk(size);
    if (*previous_program_break == -1)
    {
        return NULL;
    }
    return previous_program_break;
}