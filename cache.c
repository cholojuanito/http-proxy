#include "cache.h"

cacheList* initCache() {
    cacheList* list = Malloc(sizeof(*list));
    list->head = list->tail = NULL;
    list->lock = Malloc(sizeof(*(list->lock)));
    pthread_rwlock_init((list->lock), NULL);
    list->bytesLeft = 10000;

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

void setNode(cacheNode* node, char* key, unsigned int length) {
    if (node) {
        node->key = Malloc(sizeof(char) * length);
        memcpy(node->key, key, length);
        node->length = length;
    }
}

int addNode(cacheNode* node, cacheList* list) {
    if (list) {
        pthread_rwlock_wrlock(list->lock);
        // Make sure there is room for the next object, if not then forget don't add it
        if (node != NULL && (node->length < list->bytesLeft)) {
            
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
            pthread_rwlock_unlock(list->lock);
            return -1;
        }
        pthread_rwlock_unlock(list->lock);
    }
    return 0;
}

void deleteNode(cacheNode* node) {
    if (node) {
        // Free the arrays inside the node first then free the node up
        if (node->key) {
            free(node->key);
        }
        if (node->value) {
            free(node->value);
        }
        free(node);
    }
}

cacheNode* findNode(cacheList* list, char* key) {
    if (list) {
        cacheNode* tmp = list->head;
        while (tmp) {
            if (strcmp(tmp->key, key) == 0) {
                return tmp;
            }

            tmp = tmp->next;
        }
    }
    return NULL;
}

int readNodeContent(cacheList* list, char* key, char* value, unsigned int *len) {
    if (!list) {
        return -1;
    }

    pthread_rwlock_rdlock(list->lock);

    cacheNode* tmp = findNode(list, key);

    if (!tmp) {
        pthread_rwlock_unlock(list->lock);
        return -1;
    }

    *len = tmp->length;
    memcpy(value, tmp->value, *len);
    pthread_rwlock_unlock(list->lock);

    return 0;

}

int insertNodeContent(cacheList* list, char* key, char* value, unsigned int len) {
    if (!list) {
        return -1;
    }

    cacheNode* tmp = Malloc(sizeof(*tmp));
    initNode(tmp);
    setNode(tmp, key, len);

    if (!tmp) {
        return -1;
    }

    tmp->value = Malloc(sizeof(char) * len);
    memcpy(tmp->value, value, len);
    if (addNode(tmp, list) == -1) {
        deleteNode(tmp);
        return -2;
    }

    return 0;
}
