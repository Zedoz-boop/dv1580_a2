#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

static pthread_mutex_t memory_mutex; // 

typedef struct mem_struct {
    struct mem_struct *next;
    bool available;
    size_t size;
    void *memaddress;
} mem_struct;

static void *memorypool = NULL; // Pool for actual memory
static mem_struct *head = NULL; // Pool for block metadata

// Initialize the memory manager
void mem_init(size_t size) {
    pthread_mutex_lock(&memory_mutex);

    memorypool = malloc(size);
    head = malloc(sizeof(mem_struct));

    if (head == NULL || memorypool == NULL) {
        pthread_mutex_unlock(&memory_mutex);
        return;  // Failed to initialize memory
    }

    // Initialize the first block
    head->memaddress = memorypool;
    head->next = NULL;
    head->available = true;
    head->size = size;

    pthread_mutex_unlock(&memory_mutex);
    return;
}

// Allocate memory from the pool
void *mem_alloc(size_t size) {
    pthread_mutex_lock(&memory_mutex);

    if (size == 0) {
        pthread_mutex_unlock(&memory_mutex);
        return memorypool;  // Invalid allocation request
    }

    mem_struct *current = head;

    // Traverse the list to find a suitable block
    while (current != NULL) {
        if (current->available && current->size >= size) {
            // Check if the block can be split
            if (current->size > size) {
                mem_struct* new_block = malloc(sizeof(mem_struct));

                new_block->available = true;
                new_block->size = current->size - size;
                new_block->memaddress = current->memaddress + size;
                new_block->next = current->next;

                current->next = new_block;
                current->size = size;
                current->available = false;
            } else {
                current->available = false;
            }
            pthread_mutex_unlock(&memory_mutex);
            return (char*)current->memaddress;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&memory_mutex);
    return NULL;  // No suitable block found
}

// Free memory and coalesce adjacent free blocks
void coalesce_free_blocks() {
    mem_struct *current = head;

    while (current != NULL && current->next != NULL) {
        // Check if current block and next block are both available
        if (current->available && current->next->available) {
            // Merge current block with the next block
            current->size += current->next->size;
            
            // Skip over the next block by adjusting the 'next' pointer
            mem_struct *next_block = current->next;
            current->next = next_block->next;
            free(next_block);
        } else {
            // Move to the next block in the list
            current = current->next;
        }
    }
}

void mem_free(void* block) {
    pthread_mutex_lock(&memory_mutex);
    if (block == NULL) {
        //debug
        // printf("Warning: Trying to free a NULL pointer.\n");
        pthread_mutex_unlock(&memory_mutex);
        return;
    }

    mem_struct *current = head;

    // Traverse the linked list 
    while (current != NULL) {
        // Check if the requested block is the current block
        if (current->memaddress == block) {
            if (!current->available) {
                // Free the block
                current->available = true;
                coalesce_free_blocks();
                pthread_mutex_unlock(&memory_mutex);
                return;
            } else {
                //debug
                // printf("Error: Block at address %p is already free.\n", block);
                pthread_mutex_unlock(&memory_mutex);
                return;
            }
        }
        // Go to the next block
        current = current->next;
    }
    //debug
    // printf("Error: Block at address %p not found.\n", block);
    pthread_mutex_unlock(&memory_mutex);
    return;
}

// Resize a memory block
void *mem_resize(void *block, size_t size) {
    pthread_mutex_lock(&memory_mutex);

    if (block == NULL) {
        pthread_mutex_unlock(&memory_mutex);
        return mem_alloc(size);  // Allocate a new block if NULL
    }

    mem_struct *current = head;

    // Traverse the list to find the block
    while (current != NULL) {
        if (current->memaddress == block) {
            if (current->size >= size) {
                pthread_mutex_unlock(&memory_mutex);
                return block;  // Block is already large enough
            } else if (current->next != NULL && current->next->available &&
                       (current->size + current->next->size) >= size) {
                // Merge with the next block if it's free and large enough
                current->size += current->next->size;
                current->next = current->next->next; // Skip the next block
                free(current->next);
                pthread_mutex_unlock(&memory_mutex);
                return (char*)current->memaddress;  // Return the same block
            } else {
                mem_free(current);
                // Allocate a new block
                void *new_block = mem_alloc(size);
                if (new_block == NULL) {
                    pthread_mutex_unlock(&memory_mutex);
                    return NULL;  // Allocation failed
                }

                pthread_mutex_unlock(&memory_mutex);
                return new_block;
            }
        }
        current = current->next;
    }

    // Block not found
    pthread_mutex_unlock(&memory_mutex);
    return NULL;
}

// Deinitialize the memory manager and free the memory pools
void mem_deinit() {
    pthread_mutex_lock(&memory_mutex);

    free(memorypool); // Free the memorypool
    mem_struct *current = head; 
    // Free every strut
    while (current != NULL) {
        mem_struct *next_block = current->next;
        free(current);
        current = next_block;
    }
    // Set variables to NULL
    head = NULL;
    memorypool = NULL;

    pthread_mutex_unlock(&memory_mutex);
}