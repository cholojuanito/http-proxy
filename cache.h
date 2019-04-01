#ifndef __CACHE_H__
#define __CACHE_H__

#include "csapp.h"
#include <pthread.h>

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/*
* A cacheNode is a response from a web server
* that is connected with a particular request
*/
typedef struct cacheNode {
	char* index;
	char* content;
	struct cacheNode* prev;
	struct cacheNode* next;
	unsigned int length;
} cacheNode;

/*
* A cache is a linked list of cacheNodes
* 
*/ 
typedef struct cacheList {
	cacheNode* head;
	cacheNode* tail;
	pthread_rwlock_t* lock;
	unsigned int bytesLeft;
} cacheList;

cacheList* initCache();
void deleteCache(cacheList* list);
void initNode(cacheNode* node);
void setNode(cacheNode* node, char* index, unsigned int length);
void addNode(cacheNode* node, cacheList* list);
void deleteNode(cacheNode* node);

cacheNode* findNode(cacheList* list, char* index);
cacheNode* removeNode(char* index, cacheList* list);

int readNodeContent(cacheList* list, char* index, char* content, unsigned int len);
int insertNodeContent(cacheList* list, char* index, char* content, unsigned int len);


#endif