# 2140H-Exam
Hjemmeeksamen in2140

This project is a UDP-based communication program consisting of two parts: a lower layer and an upper layer, 
implemented in d1_udp.c and d2_lookup.c respectively. The program works as a directory service, where a request from the client is 
answered by the server in form of packets representing a directory. 
The program then prints the directory to standard output in an appropriate manner.

The lower layer discovers the address, creates and deletes a structure, D1Peer, which contains the socket, 
the address and the port of the server. Additionally there are functions for sending and receiving different packets being either
data packet or acknowledgement packets. 

The upper layer, d2, utilizes the lower layer to create a structure D2Client which in turn creates and contains a D1Peer-structure.
This layer can send a request-packet to a server which is answered with a response-size-packet indicating the number of nodes in the directory.
The server then sends a multi-packet response, which d2 receives, processes and stores in a tree-structure. Lastly d2 can print out the whole
directory in a readable format.

