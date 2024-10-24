#include "memory_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

static pthread_mutex_t memory_mutex;

typedef unsigned int uint16_t;

typedef struct Node {
    uint16_t data; // Stores the data as an unsigned 16-bit integer
    struct Node* next; // A pointer to the next node in the List
} Node;

void list_init(Node** list_head, size_t size) {
    pthread_mutex_lock(&memory_mutex);
    // Initialize memory for the list using mem_init
    mem_init(size+sizeof(Node));

    *list_head = NULL;

    //debug
    // printf("Linked list initialized with memory size: %zu bytes.\n", size);
    pthread_mutex_unlock(&memory_mutex);
}

void list_insert(Node** list_head, uint16_t data) {
    pthread_mutex_lock(&memory_mutex);
    // Allocate memory for the new node using mem_alloc
    Node* new_node = (Node*) mem_alloc(sizeof(Node));
    
    if (new_node == NULL) {
        //debug
        // printf("Failed to allocate memory for new node.\n");
        pthread_mutex_unlock(&memory_mutex);
        return;
    }

    // Set the data for the new node
    new_node->data = data;
    new_node->next = NULL; 

    // If the list is empty, make the new node the head
    if (*list_head == NULL) {
        *list_head = new_node;
        //debug
        // printf("Node with data %u inserted as the head.\n", data);
        pthread_mutex_unlock(&memory_mutex);
        return;
    }

    // Otherwise, find the last node
    Node* current = *list_head;
    while (current->next != NULL) {
        current = current->next;  // Traverse until we reach the last node
    }

    // Append the new node at the rear end
    current->next = new_node;
    //debug
    // printf("Node with data %u inserted at the rear end.\n", data);
    pthread_mutex_unlock(&memory_mutex);
}


void list_insert_after(Node* prev_node, uint16_t data) {
    pthread_mutex_lock(&memory_mutex);
    
    if (prev_node == NULL) {
        //debug
        // printf("Previus node cannot be NULL.\n");
        pthread_mutex_unlock(&memory_mutex);
        return;
    }

    // Allocate memory for the new node
    Node* new_node = (Node*) mem_alloc(sizeof(Node));

    if (new_node == NULL) {
        //debug
        // printf("Failed to allocate memory for new node.\n");
        pthread_mutex_unlock(&memory_mutex);
        return;
    }

    new_node->data = data;

    // Make the new node's next point to the previous node's next
    new_node->next = prev_node->next;

    // Make the previous node point to the new node
    prev_node->next = new_node;
    //debug
    // printf("Node with data %u inserted after node with data %u.\n", data, prev_node->data);
    pthread_mutex_unlock(&memory_mutex);
}

void list_insert_before(Node** list_head, Node* next_node, uint16_t data) {
    pthread_mutex_lock(&memory_mutex);
    if (next_node == NULL) {
        //debug
        // printf("Next node cannot be NULL.\n");
        pthread_mutex_unlock(&memory_mutex);
        return;
    }

    // Allocate memory for the new node
    Node* new_node = (Node*) mem_alloc(sizeof(Node));

    if (new_node == NULL) {
        //debug
        // printf("Failed to allocate memory for new node.\n");
        pthread_mutex_unlock(&memory_mutex);
        return;
    }

    Node* current = *list_head;

    // Check if next node is the first node
    if (next_node == *list_head) {
        // Set the new node to the head
        *list_head = new_node;
        new_node->data = data;
        new_node->next = current;
        //debug
        // printf("Node with data %u inserted as the head before node with data %u.\n", data, next_node->data);
    }
    else { 
        // Traverse the list to before the next node
        while (current->next != next_node) {
            current = current->next;  
        }
        // Set the new nodes data
        new_node->data = data;
        new_node->next = current->next;
        current->next = new_node;
    }

    //debug
    // printf("Node with data %u inserted before node with data %u.\n", data, next_node->data);
    pthread_mutex_unlock(&memory_mutex);
}

void list_delete(Node** list_head, uint16_t data) {
    pthread_mutex_lock(&memory_mutex);
    if (list_head == NULL) {
        //debug
        // printf("Cannot delete from an empty list. \n");
        pthread_mutex_unlock(&memory_mutex);
        return;
    }

    Node* current = *list_head;
    Node* prev = NULL;

    // Traverse to find the node to delete
    while (current != NULL && current->data != data) {
        prev = current;
        current = current->next;
    }

    // If node is not found
    if (current == NULL) {
        //debug
        // printf("Node with data %u not found.\n", data);
        pthread_mutex_unlock(&memory_mutex);
        return;
    }

    // If node to be deleted is the head
    if (prev == NULL) {
        *list_head = current->next;  
    } else {
        prev->next = current->next;  
    }

    // Free the memory of the deleted node using mem_free
    mem_free(current);
    //debug
    // printf("Node with data %u deleted.\n", data);
    pthread_mutex_unlock(&memory_mutex);
}

Node* list_search(Node** list_head, uint16_t data) {
    pthread_mutex_lock(&memory_mutex);
    if (*list_head == NULL) {
        //debug
        // printf("List is empty.\n");
        pthread_mutex_unlock(&memory_mutex);
        return NULL;
    }

    Node* current = *list_head;

    // Traverse the list until the end or the node is found 
    while (current != NULL) {
        if (current->data == data){
            // return node
            pthread_mutex_unlock(&memory_mutex);
            return current;
        }
        current = current->next;
    }
    // If node not found return NULL
    pthread_mutex_unlock(&memory_mutex);
    return NULL;
}

void list_display(Node** list_head) {
    pthread_mutex_lock(&memory_mutex);
    Node * current = *list_head;

    // If the list is empty
    if (current == NULL) {
        printf("[]");
        pthread_mutex_unlock(&memory_mutex);
        return;
    }

    printf("[");
    
    // Traverse the list and print each element
    while (current != NULL) {
        printf("%u", current->data);
        current = current->next;
        
        // If we are at the last node dont print ", "
        if (current != NULL) {
            printf(", ");  // Print comma after each element except the last one
        }
    }

    printf("]");
    pthread_mutex_unlock(&memory_mutex);
}

void list_display_range(Node** list_head, Node* start_node, Node* end_node) {
    pthread_mutex_lock(&memory_mutex);
    
    Node * current = *list_head;

    // If the list is empty
    if (current == NULL) {
        printf("[]");
        pthread_mutex_unlock(&memory_mutex);
        return;
    }

    printf("[");
    
    bool start = false;
    // Traverse the list and print each element
    while (current != NULL) {
        // If start_node is null set start node to the first node
        if (start_node == NULL) {
            start_node = current;
        }
        // start with start node and continue with start variabel till end or break
        if (start_node == current || start == true) {

            printf("%u", current->data);

            if (current->next != NULL && current != end_node) {
                printf(", ");  // Print comma after each element except the last one
            }  
            start = true;
        }
        // Break if we reached the end
        if (current == end_node) {
            break;
        }
      
        current = current->next;
    }

    printf("]");
    pthread_mutex_unlock(&memory_mutex);
}

int list_count_nodes(Node** list_head) {
    pthread_mutex_lock(&memory_mutex);
    Node* current = *list_head;  
    int count = 0;
    // If the list is empty
    if (current == NULL) {
        //debug
        // printf("0\n");
        pthread_mutex_unlock(&memory_mutex);
        return 0;
    }
    // Traverse the list and count
    while(current != NULL) {
        count += 1;
        current = current->next;
    }
    //debug
    // printf("The list has %u nodes. \n", count);
    pthread_mutex_unlock(&memory_mutex);
    return count;
}

void list_cleanup(Node** list_head) {
    pthread_mutex_lock(&memory_mutex);
    mem_deinit();
    // Set head to NULL after all nodes are freed
    *list_head = NULL;
    //debug
    // printf("All nodes have been cleaned up and memory has been freed.\n");
    pthread_mutex_unlock(&memory_mutex);
}