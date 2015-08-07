/*
 * Copyright (c) 2013, Mathias Brossard <mathias@brossard.org>.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file threadpool.h
 */

#ifndef THREADPOOL_HDR
#define THREADPOOL_HDR

#include <pthread.h>

typedef struct threadpool_task_tag threadpool_task_t;
typedef struct threadpool_tag threadpool_t;

/* error no. */
typedef enum {
  threadpool_invalid = -1,
  threadpool_lock_failure = -2,
  threadpool_queue_full = -3,
  threadpool_shutdown = -4,
  threadpool_thread_failure = -5
} threadpool_error_t;

/* A data structure of a task executed by a thread */
struct threadpool_task_tag {
    void (*function)(void *);
    void *argument;
};

/* A data structure of a thread pool */
struct threadpool_tag {
    pthread_mutex_t lock;     /* a lock for variables in this struct */
    pthread_cond_t notify;    /* notify a thread when a padding task exists */
    pthread_t *thread;        /* a pointer to an array of thread ids */
    threadpool_task_t *queue; /* a queue for incoming tasks */
    int thread_count;/* the number of threads */
    int queue_size;
    int head;        /* the position of the first element in the queue */
    int tail;        /* the position next to the last element in the queue */
    int task_padding_count;   /* the number of padding tasks */
    int thread_working_count; /* the number of working threads */
    int shutdown; /* an indication to check whether the pool will be */
                  /* destroyed */
};

/** 
 * @function threadpool_create
 * @brief Create a thread pool.
 * @param n_threads Number of working threads.
 * @param queue_size Size of queue.
 * @return A thread pool or NULL if fail 
 */
threadpool_t *threadpool_create(int n_threads, int queue_size);
void *threadpool_thread(void *threadpool);
int threadpool_add(threadpool_t *pool, void *function, void *argument);
int threadpool_destroy(threadpool_t *pool);
int threadpool_free(threadpool_t *pool);


#endif
