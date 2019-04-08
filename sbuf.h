#ifndef __SBUF_H__
#define __SBUF_H__

#include "csapp.h"

/* $begin sbuft */
typedef struct {
    int *buf;          /* Buffer array */         
    int n;             /* Maximum number of slots */
    int front;         /* buf[(front+1)%n] is first item */
    int rear;          /* buf[rear%n] is last item */
    sem_t mutex;       /* Protects accesses to buf */
    sem_t slots;       /* Counts available slots */
    sem_t items;       /* Counts available items */
} sbuf_t;
/* $end sbuft */

typedef struct {
    char** buf;          /* Buffer array */         
    int n;             /* Maximum number of slots */
    int front;         /* buf[(front+1)%n] is first item */
    int rear;          /* buf[rear%n] is last item */
    sem_t mutex;       /* Protects accesses to buf */
    sem_t slots;       /* Counts available slots */
    sem_t items;       /* Counts available items */
} sbuflog_t;

void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);

void sbuflog_init(sbuflog_t* sp, int n);
void sbuflog_deinit(sbuflog_t *sp);
void sbuflog_insert(sbuflog_t *sp, char* item);
char* sbuflog_remove(sbuflog_t *sp);

#endif /* __SBUF_H__ */
