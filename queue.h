#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

typedef struct work {
    int          fd;
    struct work *next;
} work_t;

typedef struct queue {
    struct work *head;
    struct work *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} queue_t;

void init_queue(queue_t *q);
void enqueue(queue_t *q, int fd);
int  get_work(queue_t *q);
void destroy_queue(queue_t *q);

#endif
