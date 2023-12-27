#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef enum { FIFO, LRU, CLOCK } Policy;

// node structure for the doubly linked list
struct Node {
    char *data;
    struct Node *next;
    struct Node *prev;
    int ref_bit;
};

// list structure
struct List {
    struct Node *head;
    struct Node *tail;
    int size;
    int num_elem;
};

// helper functions based on mitchell's psuedo
struct Cache {
    Policy policy;
    struct List *list;
    struct List *historylist;
    int clock_pointer;

    int num_compulsory_misses;
    int num_capacity_misses;
};

// function to create a new node
struct Node *create_node(char *data) {
    struct Node *new_node = (struct Node *) malloc(sizeof(struct Node));
    if (new_node == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    // strdup allocates memory for a copy of the string
    new_node->data = strdup(data);
    new_node->next = NULL;
    new_node->prev = NULL;
    new_node->ref_bit = 0;
    return new_node;
}

// function to create a new list
struct List *create_list(int size) {
    struct List *new_list = (struct List *) malloc(sizeof(struct List));
    if (new_list == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    new_list->head = NULL;
    new_list->tail = NULL;
    new_list->size = size;
    new_list->num_elem = 0;

    return new_list;
}

// function to create a new cache
struct Cache *create_cache(Policy policy, int size) {
    struct Cache *new_cache = (struct Cache *) malloc(sizeof(struct Cache));
    if (new_cache == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    new_cache->policy = policy;
    new_cache->list = create_list(size);
    new_cache->historylist = create_list(100);
    new_cache->clock_pointer = 0;
    new_cache->num_compulsory_misses = 0;
    new_cache->num_capacity_misses = 0;

    return new_cache;
}

// function to check if the list contains a specific element
bool list_contains(struct List *list, char *element) {
    struct Node *current = list->head;
    while (current != NULL) {
        if (strcmp(current->data, element) == 0) {
            return true;
        }
        current = current->next;
    }
    return false;
}

// function to check if the list is full (assuming a maximum size limit)
bool list_is_full(struct List *list) {
    return list->num_elem >= list->size;
}

// function to add a new node at the end of the list
void list_push(struct List *list, char *data) {
    struct Node *new_node = create_node(data);

    if (list->head == NULL) {
        list->head = new_node;
        list->tail = new_node;
    } else {
        new_node->prev = list->tail;
        list->tail->next = new_node;
        list->tail = new_node;
    }

    list->num_elem++;
}

// function to get the element at a specific index in the list
struct Node *list_get_element_at_index(struct List *list, char *element) {
    struct Node *current = list->head;
    while (current != NULL) {
        if (strcmp(current->data, element) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// function to set the data at a specific index in the list
void list_set_index(struct List *list, struct List *history, int index, char *element) {
    struct Node *current = list->head;
    int i = 0;
    while (current != NULL && i < index) {
        current = current->next;
        i++;
    }

    if (current != NULL) {
        list_push(history, current->data);
        free(current->data);
        current->data = strdup(element);
    }
}

// Function to remove the first node from the list
void list_pop(struct List *list, struct List *history) {
    if (list->head != NULL) {
        struct Node *temp = list->head;
        list_push(history, temp->data);
        list->head = list->head->next;
        if (list->head != NULL) {
            list->head->prev = NULL;
        }
        free(temp->data);
        free(temp);
        list->num_elem--;
    }
}

// function to move a node to the end of the list
void list_move_to_end(struct List *list, char *element) {
    struct Node *current = list_get_element_at_index(list, element);
    if (current != NULL) {
        // remove the node from its current position
        if (current->prev != NULL) {
            current->prev->next = current->next;
        } else {
            list->head = current->next;
        }

        if (current->next != NULL) {
            current->next->prev = current->prev;
        } else {
            list->tail = current->prev;
        }

        // move the node to the end
        current->prev = list->tail;
        current->next = NULL;

        if (list->tail != NULL) {
            list->tail->next = current;
            // new line to update the prev pointer
            current->prev = list->tail;
        } else {
            list->head = current;
        }

        list->tail = current;
    }
}

struct Node *list_get_element_at_index_clock(struct List *list, int index) {
    if (list == NULL) {
        fprintf(stderr, "NULL error\n");
        exit(EXIT_FAILURE);
    }

    // ensure the list is not empty
    if (list->head == NULL) {
        fprintf(stderr, "Error: List is empty\n");
        exit(EXIT_FAILURE);
    }

    // ensure the list size is positive
    if (list->size <= 0) {
        fprintf(stderr, "Error: Invalid list size\n");
        exit(EXIT_FAILURE);
    }

    struct Node *current = list->head;
    int i = 0;
    while (current != NULL && i < index) {
        current = current->next;
        i++;
    }

    if (current == NULL) {
        fprintf(stderr, "Error: Current is NULL\n");
        exit(EXIT_FAILURE);
    }

    return current;
}

// function to increment the clock pointer in the cache
void cache_increment_clock_pointer(struct Cache *cache) {
    cache->clock_pointer = (cache->clock_pointer + 1) % cache->list->size;
}

// logic for fifo, clock, lru from mitchell's section pseudo
int insert_fifo(struct Cache *cache, char *element) {
    if (cache == NULL) {
        fprintf(stderr, "Cache is NULL\n");
        exit(EXIT_FAILURE);
    }

    if (element == NULL) {
        fprintf(stderr, "Element is NULL\n");
        exit(EXIT_FAILURE);
    }

    // check if the list contains the element
    if (list_contains(cache->list, element)) {
        // return true if the element is in the cache
        return 1;
    }

    // check if the list is full
    if (list_is_full(cache->list)) {
        // check if the history of the list contains the element
        if (!list_contains(cache->historylist, element)) {
            cache->num_compulsory_misses++;
            list_push(cache->historylist, element);
        } else {
            cache->num_capacity_misses++;
        }
        list_pop(cache->list, cache->historylist);
    } else {
        cache->num_compulsory_misses++;
    }

    // add the element to the back of the list
    list_push(cache->list, element);

    // return false if the element was not in the cache
    return 0;
}

int insert_clock(struct Cache *cache, char *element) {
    if (cache == NULL) {
        fprintf(stderr, "Cache is NULL\n");
        exit(EXIT_FAILURE);
    }

    if (element == NULL) {
        fprintf(stderr, "Element is NULL\n");
        exit(EXIT_FAILURE);
    }

    // check if the list contains the element
    if (list_contains(cache->list, element)) {
        struct Node *item = list_get_element_at_index(cache->list, element);

        if (item == NULL) {
            fprintf(stderr, "item is NULL\n");
            exit(EXIT_FAILURE);
        }

        item->ref_bit = 1;

        return 1;
    }
    if (list_is_full(cache->list)) {
        struct Node *item = list_get_element_at_index_clock(cache->list, cache->clock_pointer);

        while (item->ref_bit == 1) {
            item->ref_bit = 0;
            cache_increment_clock_pointer(cache);
            item = list_get_element_at_index_clock(cache->list, cache->clock_pointer);
        }

        list_set_index(cache->list, cache->historylist, cache->clock_pointer, element);
        cache_increment_clock_pointer(cache);

        if (!list_contains(cache->historylist, element)) {
            cache->num_compulsory_misses++;
        } else {
            cache->num_capacity_misses++;
        }
        return 0;
    } else {
        cache->num_compulsory_misses++;
        list_push(cache->list, element);

        return 0;
    }
}

int insert_lru(struct Cache *cache, char *element) {
    if (cache == NULL) {
        fprintf(stderr, "Cache is NULL\n");
        exit(EXIT_FAILURE);
    }

    if (element == NULL) {
        fprintf(stderr, "Element is NULL\n");
        exit(EXIT_FAILURE);
    }

    if (list_contains(cache->list, element)) {
        list_move_to_end(cache->list, element);
        return 1;
    }

    if (list_is_full(cache->list)) {
        // remove item at front
        list_pop(cache->list, cache->historylist);

        if (!list_contains(cache->historylist, element)) {
            cache->num_compulsory_misses++;
            list_push(cache->historylist, element);
        } else {
            cache->num_capacity_misses++;
        }
    } else {
        cache->num_compulsory_misses++;
    }

    list_push(cache->list, element);
    return 0;
}

// chatgpt helped with the following, debugging and structure
Policy get_policy_enum(char *policy_str) {
    if (strcmp(policy_str, "-F") == 0) {
        return FIFO;
    } else if (strcmp(policy_str, "-L") == 0) {
        return LRU;
    } else if (strcmp(policy_str, "-C") == 0) {
        return CLOCK;
    } else {
        fprintf(stderr, "Invalid policy: %s\n", policy_str);
        exit(EXIT_FAILURE);
    }
}

// function to perform cache operations based on policy
void perform_cache_operation(struct Cache *cache, char *element) {
    int result = 0;
    switch (cache->policy) {
    case FIFO: result = insert_fifo(cache, element); break;
    case LRU: result = insert_lru(cache, element); break;
    case CLOCK: result = insert_clock(cache, element); break;
    default: fprintf(stderr, "Invalid policy: %d\n", cache->policy); exit(EXIT_FAILURE);
    }

    if (result == 1) {
        printf("HIT\n");
    } else {
        printf("MISS\n");
    }
}

int main(int argc, char *argv[]) {
    // default values
    int cache_size = 0;
    char *policy = NULL;

    // parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-N") == 0 && i + 1 < argc) {
            // get the cache size
            cache_size = atoi(argv[i + 1]);
            // skip the next argument (cache size)
            i++;
        } else {
            // assume it's the policy
            policy = argv[i];
        }
    }

    // validate arguments
    if (cache_size <= 0 || policy == NULL) {
        fprintf(stderr, "Invalid arguments. Usage: ./cacher [-N size] <policy>\n");
        exit(EXIT_FAILURE);
    }

    // convert string policy to Policy enum (default to FIFO if not specified)
    Policy new_policy = (policy != NULL) ? get_policy_enum(policy) : FIFO;

    struct Cache *cache = create_cache(new_policy, cache_size);

    // assuming a maximum element size of 255 characters
    char element[256];

    // read elements from stdin until EOF
    while (scanf("%255s", element) != EOF) {
        // perform cache operation for each element
        perform_cache_operation(cache, element);
    }

    printf("%d %d\n", cache->num_compulsory_misses, cache->num_capacity_misses);

    return 0;
}
