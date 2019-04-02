/* $begin sbufc */
#include "csapp.h"
#include "sbuf.h"

/* Create an empty, bounded, shared FIFO buffer with n slots */
/* $begin sbuf_init */
void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = Calloc(n, sizeof(int)); 
    sp->n = n;                       /* Buffer holds max of n items */
    sp->front = sp->rear = 0;        /* Empty buffer iff front == rear */
    if (sem_init(&sp->mutex, 0, 1) < 0) {
        unix_error("sem_init error MUTEX");
    }
    if (sem_init(&sp->slots, 0, n) < 0) {
        unix_error("sem_init error SLOTS");
    }
    if (sem_init(&sp->items, 0, 0) < 0) {
        unix_error("sem_init error ITEMS");
    }
}
/* $end sbuf_init */

/* Clean up buffer sp */
/* $begin sbuf_deinit */
void sbuf_deinit(sbuf_t *sp)
{
    free(sp->buf);
}
/* $end sbuf_deinit */

/* Insert item onto the rear of shared buffer sp */
/* $begin sbuf_insert */
void sbuf_insert(sbuf_t *sp, int item)
{
    if (sem_wait(&sp->slots) < 0) {
	    unix_error("sem_wait SLOTS error");
    }                       
    if (sem_wait(&sp->mutex) < 0) {
	    unix_error("sem_wait MUTEX error");
    }

    sp->buf[(++sp->rear)%(sp->n)] = item; // Write to buffer

    if (sem_post(&sp->mutex) < 0) {
	    unix_error("sem_post MUTEX error");
    }
    if (sem_post(&sp->items) < 0) {
	    unix_error("sem_post ITEMS error");
    }
}
/* $end sbuf_insert */

/* Remove and return the first item from buffer sp */
/* $begin sbuf_remove */
int sbuf_remove(sbuf_t *sp)
{
    int item;  
    if (sem_wait(&sp->items) < 0) {
	    unix_error("sem_wait ITEMS error");
    }                       
    if (sem_wait(&sp->mutex) < 0) {
	    unix_error("sem_wait MUTEX error");
    }

    item = sp->buf[(++sp->front)%(sp->n)]; // Remove the item

    if (sem_post(&sp->mutex) < 0) {
	    unix_error("sem_post MUTEX error");
    }
    if (sem_post(&sp->slots) < 0) {
	    unix_error("sem_post SLOTS error");
    }
    
    return item;
}
/* $end sbuf_remove */



void sbuflog_init(sbuflog_t *sp, int n)
{
    sp->buf = Calloc(n, sizeof(char*)); 
    sp->n = n;
    sp->front = sp->rear = 0;
    if (sem_init(&sp->mutex, 0, 1) < 0) {
        unix_error("sem_init error MUTEX");
    }
    if (sem_init(&sp->slots, 0, n) < 0) {
        unix_error("sem_init error SLOTS");
    }
    if (sem_init(&sp->items, 0, 0) < 0) {
        unix_error("sem_init error ITEMS");
    }
}

void sbuflog_deinit(sbuflog_t *sp)
{
    free(sp->buf);
}

void sbuflog_insert(sbuflog_t *sp, char* item)
{
    if (sem_wait(&sp->slots) < 0) {
	    unix_error("sem_wait SLOTS error");
    }                       
    if (sem_wait(&sp->mutex) < 0) {
	    unix_error("sem_wait MUTEX error");
    }

    sp->buf[(++sp->rear)%(sp->n)] = item;

    if (sem_post(&sp->mutex) < 0) {
	    unix_error("sem_post MUTEX error");
    }
    if (sem_post(&sp->items) < 0) {
	    unix_error("sem_post ITEMS error");
    }
}

char* sbuflog_remove(sbuflog_t *sp)
{
    char* item;  
    if (sem_wait(&sp->items) < 0) {
	    unix_error("sem_wait ITEMS error");
    }                       
    if (sem_wait(&sp->mutex) < 0) {
	    unix_error("sem_wait MUTEX error");
    }

    item = sp->buf[(++sp->front)%(sp->n)];

    if (sem_post(&sp->mutex) < 0) {
	    unix_error("sem_post MUTEX error");
    }
    if (sem_post(&sp->slots) < 0) {
	    unix_error("sem_post SLOTS error");
    }
    
    return item;
}
/* $end sbufc */

