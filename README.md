# 2140H-Exam

PROGRAM:

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

USAGE:

- call make

- run appropriate server for your test: d1_dump, d1_server or d2_server
    server needs to be run with a valid port which need to be > 1024
        example -> ./d1_server 2000

- run d1_test_client for tests on d1
    d1_test_client has two parameters: servername in form of dotted decimal or hostname and server port, which must be > 1024
        example -> ./d1_test_client localhost 2000

- run d2_test_client for test on d2
    d2_test_client has three parameters: servername, server port > 1024 and id which must be > 1000

