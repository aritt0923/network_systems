# Network Systems
My Network Systems coursework, CU Boulder, Fall 2022

Code is organized by programming assignment (PA). The directories start with the PA number because I'm lazy and I like tab completion. 

PA 1 is a reliable UDP file transfer protocol that includes both server and client. It works, but there are some things I'd like to address if I have time. 

First, it's too much code. The client and server share (at a guess) over 500 lines of identical code, all of which should be moved to a third file. 

Second, the program works by reading the binary data from the file into a dynamically allocated array, 
out of which is copies the data over to a dynamically allocated two-dimensional array, where each row represents a complete UDP packet.
Put simply, that doesn't make much sense. First, it was cumbersome to write. More importantly, it's a terrible use of heap memory. 
The only advantage to this system is that it makes finding and resending dropped packets really easy - but it is so far from neccesary. If
you're writing this program yourself, just read the data from the file into the packets as you send them. Just write a function that puts your
file offset in the correct place for a given packet. 


