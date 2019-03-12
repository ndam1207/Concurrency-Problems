# Concurrency-Problems
My playing with 2 common concurrency problems: producer-consumer and cigarette smokers in CSC 360 (Operating Systems) course at Univeristy of Victoria using POSIX threads.

1. I solved producer-consumer problem using 2 approaches: one with conditional variables and locks and another with semaphores
2. For cigarette smokers, I used the "3-pusher" approach. That is, the pushers each wait for only one ingredient provided by the agent, and then check to see if another ingredient is present and wake up the appropriate smoker.
