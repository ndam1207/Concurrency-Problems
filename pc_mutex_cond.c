#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int producer_wait_count;     // # of times producer had to wait
int consumer_wait_count;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

pthread_mutex_t mutex;
pthread_cond_t space, full; 

int items = 0;

void* producer (void* args) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    pthread_mutex_lock(&mutex);

    while(items >= MAX_ITEMS)
    {
      producer_wait_count++;
      pthread_cond_wait(&space, &mutex);
    }

    histogram[items]++;
    items++;
    assert(items >=0 && items <= MAX_ITEMS);

    pthread_cond_signal(&full);
    pthread_mutex_unlock(&mutex);
  }
  return NULL;
}

void* consumer (void* args) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    pthread_mutex_lock(&mutex);

    while(items <= 0)
    {
      consumer_wait_count++;
      pthread_cond_wait(&full, &mutex);
    }

    histogram[items]++;
    items--;
    assert(items >=0 && items <= MAX_ITEMS);
   
    pthread_cond_signal(&space);
    pthread_mutex_unlock(&mutex);
  }
  return NULL;
}

int main (int argc, char** argv) {
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&full, NULL);
  pthread_cond_init(&space, NULL);

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
  printf ("producer_wait_count=%d\nconsumer_wait_count=%d\n", producer_wait_count, consumer_wait_count);

  int sum = 0;
  for (int i = 0; i <= MAX_ITEMS; i++) {
    printf ("  items=%d, %d times\n", i, histogram [i]);
    sum += histogram [i];
  }

  assert(sum == NUM_ITERATIONS*(NUM_CONSUMERS + NUM_PRODUCERS));
  return 0;
}
