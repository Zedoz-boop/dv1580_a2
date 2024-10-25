#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

static pthread_mutex_t memory_mutex;

typedef struct mem_struct {
    struct mem_struct *next;
    bool available;
    size_t size;
    void *memaddress;
} mem_struct;

static void *memorypool = NULL; // Pool for actual memory
static mem_struct *head = NULL; // Pool for block metadata
size_t memory_available;

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
    memory_available = size;

    pthread_mutex_unlock(&memory_mutex);
    return;
}

// Allocate memory from the pool
void *mem_alloc(size_t size) {
    pthread_mutex_lock(&memory_mutex);

    if (memory_available < size) {
        pthread_mutex_unlock(&memory_mutex);
        return NULL;
    }

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
                if (new_block == NULL) {
                    pthread_mutex_unlock(&memory_mutex);
                    return NULL;
                }
                new_block->available = true;
                new_block->size = current->size - size;
                new_block->memaddress = current->memaddress + size;
                new_block->next = current->next;

                current->next = new_block;
                current->size = size;
                current->available = false;
                memory_available -= size;
            } else {
                current->available = false;
                memory_available -= current->size;
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
void mem_free(void *block) {
    if (block == NULL) {
        return;  // Returns if trying to free NULL
    }

    pthread_mutex_lock(&memory_mutex);

    mem_struct *to_free = NULL;
    mem_struct *prev = NULL;
    mem_struct *current = head;

    // Locate the block in the list
    while (current != NULL) {
        if (current->memaddress == block) {
            to_free = current;
            break;
        }
        prev = current;
        current = current->next;
    }

    // If block not found, exit
    if (to_free == NULL) {
        pthread_mutex_unlock(&memory_mutex);
        return;
    }

    // Mark block as free
    to_free->available = true;
    memory_available += to_free->size;

    // Initialize flags for coalescing checks
    mem_struct *next_block = to_free->next;
    bool free_after = (next_block != NULL && next_block->available);
    bool free_before = (prev != NULL && prev->available);

    // Coalesce based on adjacent block availability
    if (free_after && free_before) {
        // Merge previous, current, and next blocks
        prev->size += to_free->size + next_block->size;
        prev->next = next_block->next;
    } else if (free_after) {
        // Merge current and next blocks
        to_free->size += next_block->size;
        to_free->next = next_block->next;
    } else if (free_before) {
        // Merge previous and current blocks
        prev->size += to_free->size;
        prev->next = to_free->next;
    }

    pthread_mutex_unlock(&memory_mutex);
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
                pthread_mutex_unlock(&memory_mutex);
                return (char*)current->memaddress;  // Return the same block
            } else {
                // Allocate a new block
                void *new_block = mem_alloc(size);
                if (new_block == NULL) {
                    pthread_mutex_unlock(&memory_mutex);
                    return NULL;  // Allocation failed
                }

                // Copy the old data to the new block
                memcpy(new_block, block, current->size);

                // Free the old block
                mem_free(block);

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

    free(memorypool);
    mem_struct *current = head;
    while (current != NULL) {
        mem_struct *next_block = current->next;
        free(current);
        current = next_block;
    }
    head = NULL;
    memorypool = NULL;
    memory_available = 0;

    pthread_mutex_unlock(&memory_mutex);
    pthread_mutex_destroy(&memory_mutex);
}