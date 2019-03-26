#ifndef __CACHE_H__
#define __CACHE_H__

#include "csapp.h"
#include <pthread.h>

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct cacheNode {
	char* index;
	char* content;
	struct cacheNode* prev;
	struct cacheNode* next;
	unsigned int length;
} cacheNode;

typedef struct cacheList {
	cacheNode* head;
	cacheNode* tail;
	pthread_rwlock_t* lock;
	unsigned int bytes_left;
} cacheList;

cacheList* initCache();
void initNode(cacheNode* node);

void setNode(cacheNode* node, char* index, unsigned int length);
void addNode(cacheNode* node, cacheList* list);
void deleteNode(cacheNode* node);
void deleteList(cacheList* list);

cacheNode* findNode(cacheList* list, char* index);
cacheNode* removeNode(char* index, cacheList* list);

int readContentNode(cacheList* list, char* index, char* content, unsigned int len);
int insertContentNode(cacheList* list, char* index, char* content, unsigned int len);


#endif