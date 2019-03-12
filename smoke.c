#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_ITERATIONS 1000

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

int ingredients = 0;

/* {something}Handled ~ the smoker with {something} can go make a cigarette */
pthread_cond_t tobaccoHandled;
pthread_cond_t matchHandled;
pthread_cond_t paperHandled;

struct Agent {
  pthread_mutex_t mutex;
  pthread_cond_t  match;
  pthread_cond_t  paper;
  pthread_cond_t  tobacco;
  pthread_cond_t  smoke;
};

struct Agent* createAgent() {
  struct Agent* agent = malloc (sizeof (struct Agent));
  pthread_mutex_init(&agent->mutex, NULL);
  pthread_cond_init(&agent->paper, NULL);
  pthread_cond_init(&agent->match, NULL);
  pthread_cond_init(&agent->tobacco, NULL);
  pthread_cond_init(&agent->smoke, NULL);
  return agent;
}

/**
 * You might find these declarations helpful.
 *   Note that Resource enum had values 1, 2 and 4 so you can combine resources;
 *   e.g., having a MATCH and PAPER is the value MATCH | PAPER == 1 | 2 == 3
 */
enum Resource            {    MATCH = 1, PAPER = 2,   TOBACCO = 4};
char* resource_name [] = {"", "match",   "paper", "", "tobacco"};

int signal_count [5];  // # of times resource signalled
int smoke_count  [5];  // # of times smoker with resource smoked

/**
 * This is the agent procedure.  It is complete and you shouldn't change it in
 * any material way.  You can re-write it if you like, but be sure that all it does
 * is choose 2 random reasources, signal their condition variables, and then wait
 * wait for a smoker to smoke.
 */
void* agent (void* av) {
  struct Agent* a = av;
  static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};
  static const int matching_smoker[] = {TOBACCO,     PAPER,         MATCH};
  
  pthread_mutex_lock (&a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
      int r = random() % 3;
      signal_count [matching_smoker [r]] ++;
      int c = choices [r];
      if (c & MATCH) {
        VERBOSE_PRINT ("match available\n");
        pthread_cond_signal (&a->match);
      }
      if (c & PAPER) {
        VERBOSE_PRINT ("paper available\n");
        pthread_cond_signal (&a->paper);
      }
      if (c & TOBACCO) {
        VERBOSE_PRINT ("tobacco available\n");
        pthread_cond_signal (&a->tobacco);
      }
      VERBOSE_PRINT ("agent is waiting for smoker to smoke\n");
      pthread_cond_wait (&a->smoke, &a->mutex);
    }
  pthread_mutex_unlock (&a->mutex);
  return NULL;
}


void* tobacco_handler(void* agent)
{ 
  struct Agent* a = agent; 
  pthread_mutex_lock(&a->mutex);

  for (;;) {
    pthread_cond_wait(&a->tobacco, &a->mutex);
    VERBOSE_PRINT("tobacco handler!\n");
    ingredients |= TOBACCO;

    if (!(ingredients ^ (TOBACCO | PAPER)))  /* if tobacco and paper are on the table */
    {
      ingredients = 0;  /* Reset ingredients, prepare for next turn */
      pthread_cond_signal(&matchHandled);
    }

    else if (!(ingredients ^ (TOBACCO | MATCH)))
    {
      ingredients = 0;
      pthread_cond_signal(&paperHandled);
    }
  }
  pthread_mutex_unlock(&a->mutex);  

  return NULL;
}

void* paper_handler(void *agent)
{
  struct Agent* a = agent; 
  pthread_mutex_lock(&a->mutex);

  for (;;) {
    pthread_cond_wait(&a->paper, &a->mutex);

    VERBOSE_PRINT("paper handler!\n");

    ingredients |= PAPER;

    if (!(ingredients ^ (PAPER | TOBACCO)))
    {
      ingredients = 0;
      pthread_cond_signal(&matchHandled);
    }

    else if (!(ingredients ^ (PAPER | MATCH)))
    {
      ingredients = 0;
      pthread_cond_signal(&tobaccoHandled);
    }
  }

  pthread_mutex_unlock(&a->mutex);  

  return NULL;
}

void* match_handler(void* agent)
{
  struct Agent* a = agent; 
  pthread_mutex_lock(&a->mutex);

  for (;;) {
    pthread_cond_wait(&a->match, &a->mutex);

    VERBOSE_PRINT("match handler!\n");
    ingredients |= MATCH;

    if (!(ingredients ^ (MATCH | PAPER)))
    {
      ingredients = 0;
      pthread_cond_signal(&tobaccoHandled);
    }

    else if (!(ingredients ^ (MATCH | TOBACCO)))
    {
      ingredients = 0;
      pthread_cond_signal(&paperHandled);
    }
  }

  pthread_mutex_unlock(&a->mutex); 

  return NULL;
}

void* smoker_with_tobacco(void* agent)
{  
  struct Agent* a = agent;
  pthread_mutex_lock(&a->mutex);

  for (;;) {
    pthread_cond_wait(&tobaccoHandled, &a->mutex);

    smoke_count[TOBACCO]++;
    VERBOSE_PRINT("tobacco smoker!\n");

    pthread_cond_signal(&a->smoke);
  }

  pthread_mutex_unlock(&a->mutex);

  return NULL;
}

void* smoker_with_paper(void* agent)
{
  struct Agent* a = agent;
  pthread_mutex_lock(&a->mutex);

  for (;;) {
    pthread_cond_wait(&paperHandled, &a->mutex);
    VERBOSE_PRINT("paper smoker!\n");
    smoke_count[PAPER]++;

    pthread_cond_signal(&a->smoke);
  } 

  pthread_mutex_unlock(&a->mutex);

  return NULL;
}

void* smoker_with_match(void *agent)
{
  struct Agent* a = agent;
  pthread_mutex_lock(&a->mutex);

  for (;;) {
    pthread_cond_wait(&matchHandled, &a->mutex);

    smoke_count[MATCH]++;
    VERBOSE_PRINT("match smoker!\n");

    pthread_cond_signal(&a->smoke);
  } 
  pthread_mutex_unlock(&a->mutex);
  return NULL;
}

int main (int argc, char** argv) {
  struct Agent* a = createAgent();
  
  pthread_cond_init(&tobaccoHandled, NULL);
  pthread_cond_init(&paperHandled, NULL);
  pthread_cond_init(&matchHandled, NULL);

  pthread_t match_smoker, paper_smoker, tobacco_smoker;

  pthread_create(&match_smoker, NULL, smoker_with_match, a);
  pthread_create(&paper_smoker, NULL, smoker_with_paper, a);
  pthread_create(&tobacco_smoker, NULL, smoker_with_tobacco, a);

  pthread_t match, paper, tobacco;

  pthread_create(&match, NULL, match_handler, a);
  pthread_create(&paper, NULL, paper_handler, a);
  pthread_create(&tobacco, NULL, tobacco_handler, a);

  pthread_t ag;

  pthread_create(&ag, NULL, agent, a);
  pthread_join (ag, NULL);
  
  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);

  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);
}

