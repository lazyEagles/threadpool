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

#include <stdio.h>
#include <stdlib.h>
#include "threadpool.h"

threadpool_t *threadpool_create(int n_threads, int queue_size) {
  threadpool_t *pool;
  int mr, cr, i, tr;

  pool = malloc(sizeof(threadpool_t));
  if (!pool) {
    fprintf(stderr, "Error: pool malloc fails.\n");
    goto pool_err;
  }
  /* initialize */
  pool->thread_working_count = 0;
  pool->queue_size = queue_size;
  pool->head = 0;
  pool->tail = 0;
  pool->shutdown = 0;
  mr = pthread_mutex_init(&(pool->lock), NULL);
  if (mr) {
    fprintf(stderr, "Error: %d. mutex init fails.\n", mr);
    goto mutex_err;
  }
  cr = pthread_cond_init(&(pool->notify), NULL);
  if (cr) {
    fprintf(stderr, "Error: %d. cond init failes.\n", cr);
    goto cond_err;
  }
  pool->thread = malloc(n_threads * sizeof(pthread_t));
  if (!pool->thread) {
    fprintf(stderr, "Error: thread malloc fails.\n");
    goto thread_err;
  }
  pool->queue = malloc(queue_size * sizeof(threadpool_task_t));
  if (!pool->queue) {
    fprintf(stderr, "Error: queue malloc fails.\n");
    goto queue_err;
  }

  for (i = 0; i < n_threads; i++) {
    /* create thread to run task */
    tr = pthread_create(&pool->thread[i], NULL, threadpool_thread, 
                        (void *)pool);
    if (tr) {
      threadpool_destroy(pool);
      return NULL;
    }
    pool->thread_count++;
    pool->thread_working_count++;
  }

  return pool;

queue_err:
thread_err:
cond_err:
mutex_err:
pool_err:
  if (pool) {
    threadpool_free(pool);
  }
  return NULL;
}

/* a thread to run a task from queue */
void *threadpool_thread(void *threadpool)
{
  threadpool_t *pool = (threadpool_t *)threadpool;
  threadpool_task_t task;
  for (;;) {
    pthread_mutex_lock(&pool->lock);

    /* wait for a task if not shut down */
    while (pool->task_padding_count == 0 && !pool->shutdown) {
        pthread_cond_wait(&pool->notify, &pool->lock);
    }
    /* shut down when no padding task */
    if (pool->shutdown == 1 && pool->task_padding_count == 0) {
        break;
    }
    task = pool->queue[pool->head];
    pool->head++;
    pool->head = (pool->head == pool->queue_size) ? 0 : pool->head;
    pool->task_padding_count--;

    pthread_mutex_unlock(&(pool->lock));

    (*task.function)(task.argument);
  }
  pool->thread_working_count--;
  pthread_mutex_unlock(&(pool->lock));
  pthread_exit(NULL);
}

int threadpool_add(threadpool_t *pool, void *function, void *argument)
{
  int errno;
  /* pool and function exist? */
  if (!pool || !function) {
    return threadpool_invalid;
  }
  /* get lock */
  if (pthread_mutex_lock(&pool->lock)) {
    return threadpool_lock_failure;
  }
  /* shut down? */
  if (pool->shutdown) {
    errno = threadpool_shutdown;
    goto err;
  }
  /* queue full? */
  if (pool->thread_working_count == pool->queue_size) {
    errno = threadpool_queue_full;
    goto err;
  }
  pool->queue[pool->tail].function = function;
  pool->queue[pool->tail].argument = argument;
  pool->tail++;
  pool->tail = (pool->tail == pool->queue_size) ? 0 : pool->tail;
  pool->task_padding_count++;
  if (pthread_cond_signal(&pool->notify) || 
      pthread_mutex_unlock(&pool->lock)) {
    return threadpool_lock_failure;
  }

  return 0;
err:
  if (pthread_mutex_unlock(&pool->lock)) {
    return threadpool_lock_failure;
  }
  return errno;
}

int threadpool_destroy(threadpool_t *pool)
{
  int i, errno;
  /* lock fails ? */
  if (pthread_mutex_lock(&pool->lock)) {
    errno = threadpool_lock_failure;
    goto err;
  }
  /* already shutdown ? */
  if (pool->shutdown) {
    errno = threadpool_shutdown;
    goto err;
  }
  pool->shutdown = 1;
  /* unlock fails ? */
  if (pthread_cond_broadcast(&pool->notify) ||
      pthread_mutex_unlock(&pool->lock)) {
    errno = threadpool_lock_failure;
    goto err;
  }
  /* join threads */
  for (i = 0; i < pool->thread_count; i++) {
    if (pthread_join(pool->thread[i], NULL)) {
      errno = threadpool_thread_failure;
      goto err;
    }
  }
  /* free thread pool */
  threadpool_free(pool);
  return 0;
err:
  return errno;
}

int threadpool_free(threadpool_t *pool)
{
  if (!pool || pool->thread_working_count > 0) {
    fprintf(stderr, "Error: pool is NULL or working threads > 0.\n");
    return -1;
  }
  if (pool->thread) {
    /* free allocated memory */
    free(pool->thread);
    pool->thread = NULL;
    free(pool->queue);
    pool->queue = NULL;
    /* destory lock and cond var */
    pthread_mutex_lock(&pool->lock);
    pthread_cond_destroy(&pool->notify);
    pthread_mutex_unlock(&pool->lock);
    pthread_mutex_destroy(&pool->lock);
  }
  free(pool);
  pool = NULL;
  return 0;
}
