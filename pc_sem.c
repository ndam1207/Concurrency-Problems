#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <assert.h>

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int producer_wait_count;     // # of times producer had to wait
int consumer_wait_count;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

sem_t mutex;
sem_t space; 
sem_t full;

int items = 0;

void* producer (void* args) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    sem_wait(&space);
    sem_wait(&mutex);

    histogram[items]++;
    items++;
    assert(items >=0 && items <= MAX_ITEMS);

    sem_post(&mutex);
    sem_post(&full);
  }
  return NULL;
}

void* consumer (void* args) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    sem_wait(&full);
    sem_wait(&mutex);

    histogram[items]++;
    items--;
    assert(items >=0 && items <= MAX_ITEMS);

    sem_post(&mutex);
    sem_post(&space);
  }
  return NULL;
}

int main (int argc, char** argv) {
  sem_init(&mutex, 0, 1);
  sem_init(&space, 0, MAX_ITEMS);
  sem_init(&full, 0, 0);

  pthread_t prods[NUM_PRODUCERS];
  pthread_t cons[NUM_CONSUMERS];
  
  for(int i = 0; i < NUM_PRODUCERS; i++)
  {
    pthread_create(&prods[i], NULL, producer, NULL);
  }

  for(int i = 0; i < NUM_CONSUMERS; i++)
  {
    pthread_create(&cons[i], NULL, consumer, NULL);
  }

  for(int i = 0; i < NUM_PRODUCERS; i++)
  {
    pthread_join(prods[i], NULL);
  }

  for(int i = 0; i < NUM_CONSUMERS; i++)
  {
    pthread_join(cons[i], NULL);
  }

  int sum = 0;
  for (int i = 0; i <= MAX_ITEMS; i++) {
    printf ("  items=%d, %d times\n", i, histogram [i]);
    sum += histogram [i];
  }

  assert(sum == NUM_ITERATIONS*(NUM_CONSUMERS + NUM_PRODUCERS));
  return 0;
}
