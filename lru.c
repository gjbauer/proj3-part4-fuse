#include <stdlib.h>
#include "lru.h"
#include "cache.h"

// Add a cache entry index to the head of the LRU (Least Recently Used) list
// The LRU list tracks cache usage order for eviction decisions
// Returns pointer to the new LRU node
LRU_List *lru_push(cache *cache, int index)
{
	LRU_List *list = cache->lru;
	// Allocate new node
	LRU_List *node = (LRU_List*)malloc(sizeof(struct LRU_List));
	node->index = index;
	
	if (cache->lru_size>0 && list != NULL)
	{
		// Insert into existing circular doubly-linked list
		node->next = list;
		node->prev = list->prev;
		list->prev = node;
		list->prev->next = node;
	}
	else
	{
		// First node - create circular links to self
		node->next = node;
		node->prev = node;
	}
	
	cache->lru_size++;
	return node;
}

// Remove the least recently used entry from the LRU list
// Returns the cache entry index of the evicted item
// Securely wipes the removed node before freeing
int64_t lru_pop(cache *cache, LRU_List *list)
{
    int64_t index;
    
    if (list == NULL) {
        return -1;  // Invalid node
    }
    
    // Store the index before freeing
    index = list->index;
    
    if (cache->lru_size == 1) {
        // Only one node in the list
        cache->lru = NULL;
        arc4random_buf(list, sizeof(struct LRU_List));
        free(list);
    } else {
        // Remove this node from the circular list
        list->prev->next = list->next;
        list->next->prev = list->prev;
        
        // If we're removing the head, update cache->lru to the next node
        if (list == cache->lru) {
            cache->lru = list->next;
        }
        
        // Securely overwrite node data before freeing
        arc4random_buf(list, sizeof(struct LRU_List));
        free(list);
    }
    
    cache->lru_size--;
    
    return index;
}

