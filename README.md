# Network Systems

My Network Systems coursework, CU Boulder, Fall 2022

Code is organized by programming assignment (PA). 

### PA 1 

Reliable UDP file transfer protocol that includes both server and client.

##### Notes

Too much code - only two files were allowed to be submitted for this assignment, a server and a client, so there is a ton of redundant code between them. The program could also make better use of heap memory. 

### PA 2 

Multi-threaded TCP server capable of serving requests in parallel. 

### PA 3 

Multi-threaded, caching, TCP proxy server. 

##### Notes

Largely written on top of PA-2. The caching mechanism is a thread safe hash-table with linked list chaining. The head of each linked list contains a reader-writer semaphore that allows unlimited readers to simultainously access the list, but allows access to only one writer. 

### PA 4

Meant to be a distributed file system, but is not complete. Probably not much useful code here. 
