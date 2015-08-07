
#include <stdio.h>
#include <stdlib.h>
#include "threadpool.h"

#define NUM 100
#define THREADS 10
#define QUEUE 1024

void printarray(void *arg);
 
int main() {
  int i;
  int a[NUM];
  threadpool_t *pool;
  for (i = 0; i < NUM; i++) {
    a[i] = i;
  } 
  pool = threadpool_create(THREADS, QUEUE);
  if (!pool) {
    fprintf(stderr, "ERROR: fails to create thread pool.\n");
    exit(1);
  }
  for (i = 0; i < NUM; i++) {
    threadpool_add(pool, printarray, &a[i]);
  }
  threadpool_destroy(pool);

  return 0;
}

void printarray(void *arg) {
  int *i = (int *)arg;
  printf("%d\n", *i);
}
