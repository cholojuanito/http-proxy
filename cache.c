#include "cache.h"

cacheList* initCache() {
    cacheList* list = Malloc(sizeof(*list));
    list->head = list->tail = NULL;
    list->lock = Malloc(sizeof(*(list->lock)));
    pthread_rwlock_init((list->lock), NULL);
    list->bytes_left = MAX_CACHE_SIZE;

    return list;
}

void initNode(cacheNode* node) {
    if(node) {
        node->next = NULL;
        node->prev = NULL;
        node->length = 0;
	}
}

void setNode(cacheNode* node, char* index, unsigned int length) {

}

void addNode(cacheNode* node, cacheList* list) {

}

void deleteNode(cacheNode* node) {

}

void deleteList(cacheList* list) {

}

cacheNode* findNode(cacheList* list, char* index) {

}

cacheNode* removeNode(char* index, cacheList* list) {

}


int readContentNode(cacheList* list, char* index, char* content, unsigned int len) {

}

int insertContentNode(cacheList* list, char* index, char* content, unsigned int len) {
    
}
