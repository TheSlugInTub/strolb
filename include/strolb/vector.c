#include <strolb/vector.h>
#include <assert.h>

// Initialize a new vector with specified element size
slb_Vector* slb_Vector_Create(size_t elemSize,
                              size_t initial_capacity)
{
    slb_Vector* vector = (slb_Vector*)malloc(sizeof(slb_Vector));
    if (vector == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for vector.\n");
        assert(1);
        return NULL;
    }

    vector->data = malloc(initial_capacity * elemSize);
    if (vector->data == NULL)
    {
        free(vector);
        fprintf(stderr,
                "Failed to allocate memory for vector data.\n");
        assert(1);
        return NULL;
    }

    vector->elemSize = elemSize;
    vector->size = 0;
    vector->capacity = initial_capacity;
    return vector;
}

// Add an element to the vector (copies the element)
int slb_Vector_PushBack(slb_Vector* vector, const void* element)
{
    // Resize if needed
    if (vector->size >= vector->capacity)
    {
        size_t new_capacity = vector->capacity * 2;
        void*  new_data =
            realloc(vector->data, new_capacity * vector->elemSize);
        if (new_data == NULL)
        {
            fprintf(stderr,
                    "Failed to reallocate memory for vector data.\n");
            assert(1);
            return -1;
        }

        vector->data = new_data;
        vector->capacity = new_capacity;
    }

    // Calculate position to insert and copy the element
    char* target =
        (char*)vector->data + (vector->size * vector->elemSize);
    memcpy(target, element, vector->elemSize);

    vector->size++;
    return 0;
}

// Get a pointer to an element
void* slb_Vector_Get(slb_Vector* vector, size_t index)
{
    if (index >= vector->size)
    {
        fprintf(stderr, "Index out of bounds.\n");
        assert(1);
        return NULL;
    }
    return (char*)vector->data + (index * vector->elemSize);
}

int slb_Vector_Resize(slb_Vector* vector, size_t new_size)
{
    if (vector == NULL)
    {
        fprintf(stderr, "Vector is NULL.\n");
        assert(1);
        return -1;
    }

    // If we need more capacity, reallocate
    if (new_size > vector->capacity)
    {
        size_t new_capacity = new_size;
        // Optionally use growth factor for future allocations
        if (new_capacity < vector->capacity * 2)
        {
            new_capacity = vector->capacity * 2;
        }

        void* new_data =
            realloc(vector->data, new_capacity * vector->elemSize);
        if (new_data == NULL)
        {
            fprintf(
                stderr,
                "Failed to reallocate memory for vector resize.\n");
            assert(1);
            return -1;
        }

        vector->data = new_data;
        vector->capacity = new_capacity;
    }

    // Update the size
    vector->size = new_size;
    return 0;
}

// Remove an element by index (shifts subsequent elements)
int slb_Vector_Remove(slb_Vector* vector, size_t index)
{
    if (index >= vector->size)
    {
        fprintf(stderr, "Index out of bounds.\n");
        assert(1);
        return -1;
    }

    // Calculate position to remove
    char* pos_to_remove =
        (char*)vector->data + (index * vector->elemSize);

    // Calculate position of element after the one to remove
    char* next_pos = pos_to_remove + vector->elemSize;

    // Calculate number of bytes to move
    size_t bytes_to_move =
        (vector->size - index - 1) * vector->elemSize;

    // Shift elements
    if (bytes_to_move > 0)
    {
        memmove(pos_to_remove, next_pos, bytes_to_move);
    }

    vector->size--;
    return 0;
}

// Free the memory used by the vector
void slb_Vector_Free(slb_Vector* vector)
{
    free(vector->data);
    free(vector);
}

void slb_Vector_Clear(slb_Vector* vector)
{
    vector->size = 0;
}

void slb_Vector_Erase(slb_Vector* vector, int index)
{
    if (index < 0 || index >= vector->size)
        return;

    // Shift elements down
    for (int i = index; i < vector->size - 1; i++)
    {
        memcpy(slb_Vector_Get(vector, i),
               slb_Vector_Get(vector, i + 1), vector->elemSize);
    }
    vector->size--;
}

int slb_Vector_Insert(slb_Vector* vector, size_t index,
                      const void* element)
{
    if (vector == NULL)
    {
        fprintf(stderr, "Vector is NULL.\n");
        assert(1);
        return -1;
    }

    if (index > vector->size)
    {
        fprintf(stderr, "Index out of bounds for insertion.\n");
        assert(1);
        return -1;
    }

    // Resize if needed
    if (vector->size >= vector->capacity)
    {
        size_t new_capacity = vector->capacity * 2;
        void*  new_data =
            realloc(vector->data, new_capacity * vector->elemSize);
        if (new_data == NULL)
        {
            fprintf(stderr, "Failed to reallocate memory for vector "
                            "insertion.\n");
            assert(1);
            return -1;
        }

        vector->data = new_data;
        vector->capacity = new_capacity;
    }

    // Calculate position to insert
    char* pos_to_insert =
        (char*)vector->data + (index * vector->elemSize);

    // Calculate number of bytes to move (shift elements after index)
    size_t bytes_to_move = (vector->size - index) * vector->elemSize;

    // Shift elements to the right if needed
    if (bytes_to_move > 0)
    {
        char* move_dest = pos_to_insert + vector->elemSize;
        memmove(move_dest, pos_to_insert, bytes_to_move);
    }

    // Copy the new element into the position
    memcpy(pos_to_insert, element, vector->elemSize);

    vector->size++;
    return 0;
}
