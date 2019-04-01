#include "cache.h"

cacheList* initCache() {
    cacheList* list = Malloc(sizeof(*list));
    list->head = list->tail = NULL;
    list->lock = Malloc(sizeof(*(list->lock)));
    pthread_rwlock_init((list->lock), NULL);
    list->bytesLeft = MAX_CACHE_SIZE;

    return list;
}

void deleteCache(cacheList* list) {
    if (list) {
        cacheNode* tmpHead = list->head;
        while (tmpHead) {
            cacheNode* tmp = tmpHead;
            tmpHead = tmp->next;
            deleteNode(tmp);
        }
        pthread_rwlock_destroy(list->lock);
        free(list->lock);
        free(list);
    }

}

void initNode(cacheNode* node) {
    if (node) {
        node->next = NULL;
        node->prev = NULL;
        node->length = 0;
	}
}

void setNode(cacheNode* node, char* index, unsigned int length) {
    if (node) {
        node->index = Malloc(sizeof(char) * length);
        memcpy(node->index, index, length);
        node->length = length;
    }
}

int addNode(cacheNode* node, cacheList* list) {
    if (list) {
        pthread_rwlock_wrlock(list->lock);
        // Make sure there is room for the next object, if not then forget don't add it
        if (node && (list->bytesLeft < node->length) && (node->length < MAX_OBJECT_SIZE)) {
            
            if (!list->tail) {
                list->head = list->tail = node;                
            }
            else {
                list->tail->next = node;
                node->prev = list->tail;
                list->tail = node;
            }
            
            list->bytesLeft -= node->length;
        }
        else {
            return -1;
        }
        pthread_rwlock_unlock(list->lock);
    }
}

void deleteNode(cacheNode* node) {
    if (node) {
        // Free the arrays inside the node first then free the node up
        if (node->index) {
            free(node->index);
        }
        if (node->content) {
            free(node->content);
        }
        free(node);
    }
}

cacheNode* findNode(cacheList* list, char* index) {
    if (list) {
        cacheNode* tmp = list->head;
        while (tmp) {
            if (strcmp(tmp->index, index) == 0) {
                return tmp;
            }

            tmp = tmp->next;
        }
    }
    return NULL;
}

cacheNode* removeNode(char* index, cacheList* list) {

}


int readNodeContent(cacheList* list, char* index, char* content, unsigned int *len) {
    if (!list) {
        return -1;
    }

    pthread_rwlock_rdlock(list->lock);

    cacheNode* tmp = findNode(list, index);

    if (!tmp) {
        pthread_rwlock_unlock(list->lock);
        return -1;
    }

    *len = tmp->length;
    memcpy(content, tmp->content, *len);
    pthread_rwlock_unlock(list->lock);

    return 0;

}

int insertNodeContent(cacheList* list, char* index, char* content, unsigned int len) {
    if (!list) {
        return -1;
    }

    cacheNode* tmp = Malloc(sizeof(*tmp));
    initNode(tmp);
    setNode(tmp, index, len);

    if (!tmp) {
        return -1;
    }

    tmp->content = Malloc(sizeof(char) * len);
    memcpy(tmp->content, content, len);
    if (addNode(tmp, list) == - 1) {
        return -2;
    }

    return 0;
}
