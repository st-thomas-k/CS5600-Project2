#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "queue.h"

void init_queue(queue_t *q) {
    q->head = NULL;
    q->tail = NULL;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

void enqueue(queue_t *q, int fd) {

    work_t* work_item = malloc(sizeof(work_t));
    if (work_item == NULL) {
        perror("malloc failed\n");
        exit(1);
    }

    work_item->fd = fd;
    work_item->next = NULL;

    pthread_mutex_lock(&(q->mutex));

    if (q->tail == NULL) {
        q->head = work_item;
        q->tail = work_item;
    }
    else {
      q->tail->next = work_item;
      q->tail = work_item;
    }

    pthread_cond_signal(&(q->cond));
    pthread_mutex_unlock(&(q->mutex));
}



void destroy_queue(queue_t *q) {
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
}