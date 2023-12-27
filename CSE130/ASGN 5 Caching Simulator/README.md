#Assignment 5 directory

In this assignment, you will be implementing a cache. Your cache must support first-in-first-out (FIFO), least-recently-used (LRU), and clock eviction policies. Your program should continuously take items from stdin until stdin is closed. After each lookup, your program should print to stdout specifying whether the item that was accessed is a HIT or MISS . If the lookup was a miss, your cache will add the item to its cache and evict an item based upon the eviction policy that the user specified. Before your program exits, and after stdin is closed, you must include a summary line that specifies the total number of compulsory and capacity misses. 

Note: the eviction algorithms are fully associative, so thare are no conflict misses.