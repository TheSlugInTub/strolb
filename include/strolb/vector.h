#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    void** data; // Array of void pointers to store any type
    size_t elemSize;
    size_t size;     // Current number of elements in the vector
    size_t capacity; // Amount of data allocated for the vector
} slb_Vector;

// Initialize the vector
slb_Vector* slb_Vector_Create(size_t elemSize, size_t initial_capacity);

// Push an element into a vector
int slb_Vector_PushBack(slb_Vector* vector, const void* element);

// Get the data at an index of the vector
void* slb_Vector_Get(slb_Vector* vector, size_t index);

// Resizes a vector, allocating memory for [new_size] elements
int slb_Vector_Resize(slb_Vector* vector, size_t new_size);

// Remove an element from the vector by index
int slb_Vector_Remove(slb_Vector* vector, size_t index);

// Free the memory used by the vector
void slb_Vector_Free(slb_Vector* vector);

// Set the size to zero, the memory remains allocated for future use
void slb_Vector_Clear(slb_Vector* vector);

void slb_Vector_Erase(slb_Vector* vector, int index);

int slb_Vector_Insert(slb_Vector* vector, size_t index,
                      const void* element);
