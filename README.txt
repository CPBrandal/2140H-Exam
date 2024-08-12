Home-exam IN-2140

Candidate number: 15221 
--------------------------------------------------------------------------------------------------------------------------------------
THE PROGRAM:

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
--------------------------------------------------------------------------------------------------------------------------------------
USAGE:

- call make

- run appropriate server for your test: d1_dump, d1_server or d2_server
    server needs to be run with a valid port which need to be > 1024
        example -> ./d1_server 2000

- run d1_test_client for tests on d1
    d1_test_client has two parameters: servername in form of dotted decimal or hostname and server port, which must be > 1024

- run d2_test_client for test on d2
    d2_test_client has three parameters: servername, server port > 1024 and id which must be > 1000
--------------------------------------------------------------------------------------------------------------------------------------

D1:

typedef enum Error{
    ERR_INVALID_ARGUMENT = -2, // Invalid argument for function/method
    ERR_COULD_NOT_GET_IP = -3, // Unable to get correct ip when trying to convert from dotted decimal and hostname
    ERR_RECV_NOT_MATCHING_EXPECTED = -4,    // Bytes received does not match expected amount
    ERR_SENDTO_NOT_MATCHING_EXPECTED = -5, // Bytes sent does not match expected amount
    ERR_WRONG_ACK_RECEIVED = -6 , // Received wrong ack
    ERR_WRONG_CHECKSUM = -7,  // Received wrong checksum
    ERR_INVALID_SEQNO = -8,     // Sequence number is invalid
    ERR_ACK_SEND_FAILED = -9, // Sending ack failed ack-size deviates from expected size 
    ERR_HEADER_SIZE_MISMATCH = -10, // Size specified by packet header does not match received bytes
    ERR_NOT_PACKETRESPONSESIZE = -11 // Packet received was not of type Packet-Response-Size
} ErrorCode;


void errorToString( ErrorCode code );

    I have done zero modifications to the D1Header struct and the D1Peer struct.

    I have created an enum representing different error-types which helps indicating specific errors.
    It is implemented in d1_udp_mod.h.
    Additionally I created a function errorToString which prints an appropriate error message for different error-cases. 
    This gives me an easy way to print spesific error messages. I have added these print messages throughout the program
    to make it easier to identify where and why the errors occur.
    Because most of the functions are supposed to return a negative value on failure everything in the enum is set to a negative value. 
    If one were to further expand the program the error codes returned can help differentiate between errors, and make it easier to handle specific errors.

    The errors which can be printed with perror are done so. Among others this include malloc, sendto, recvfrom. 

    #include <sys/time.h> is for socket timer.
    #include <errno.h> to distinguish between error caused by timer running out or recvfrom in d1_wait_ack.


int create_socket: 
    I have made this function to create a socket for the client using ipv4-adress and UDP. 
    It is a help function and is only called by d1_create_client.
    Returns socket in case of success, and -1 in case of failure.


D1Peer* d1_create_client:
    Very basic, creating a D1Peer client and calling create_socket().
    Returns client in case of success and NULL in case of failure.


D1Peer* d1_delete (D1Peer* peer):
    Free all allocated memory and close socket.
    I added a print to let the user know if the client has been deleted.
    Always returns NULL.


int d1_get_peer_info (struct D1Peer* client, const char* servername, uint16_t server_port):
    This function is used to discover server address, and set up the server port for the client. 
    The function can discover servername given as dotted decimal or hostname. 
    I have decided to set a lower limit for serverport at 1024 given that ports under 1024 are serverports.
    Returns 1 in case of success and 0 in case of failure. Prints appropriate error-msg.


uint16_t make_checksum (D1Header* header, char* buffer, size_t sz):
    I made a function for creating a checksum given a header, buffer and size. It is also used to check if 
    received checksum is correct. When I did xor operations on the payload, the 16bit of two characters was stored in reverse 
    in memory so i had to reverse them back. Meaning that if payload was "connect", when i extract two and two chars, 
    I would get "oc", "nn", "ce" and "t".
    I assume that header is not NULL since funtion is called by recv_data and send_data.
    I assume that if buffer is NULL or if sz is 0 then it is an ACK packet and I return 
    checksum xor-ed with header. 

    I assume that the sz is correct for the buffer because it is called from d1_send where I 
    assume the user has written correct sz, and from d1_recv_data where I check if size is correct, 
    and therefore I do not need to check for buffer overflow.

    The function returns uint16_t because that is the size of header->checksum.
    I also assumed that it is unnecessary to checksum an ACK packet, which seems correct given the tests,
    however I still do it when I send ACK packets because I saw that the d1_server example output had done it.
    On the other hand when I receive an ACK i do not check the checksum, since we are only looking at one specific bit.
    I figure that the checksum is there to validate if the data in the payload has been tampered with. An ack-packet has no "data".
    Always returns an uint16_t checksum.


int d1_send_data (struct D1Peer* peer, char* buffer, size_t sz):
    This function is pretty standard. I use int instead of ssize_t to represent bytes_sent, this is because the function returns an int,
    and the function is supposed to return bytes_sent in case of success. Also an int is large enough to represent the sendto in our program.
    I assume that the user sends correct sz for the buffer.
    Returns bytes_sent in case of success, and appropriate error in case of failure.


int toggle_socket_timer (int socket, int enable, int sec, long usec):
    This function toggles whether the socket has a timer while it is closed or not. My interpretation of the task was that d1_wait_ack 
    should block and wait for 1 second, hence the timer. However I do not want a timer on the socket at all times so I toggle it.
    Returns 0 in case of success and -1 in case of failure.


int d1_wait_ack (D1Peer* peer, char* buffer, size_t sz):
    d1_wait_ack is called by d1_send_data. The function should try to receive an ACK, wait max 1 second, validate
    whether the ACK is correct. If ACK is not correct we should call d1_send_data only one more time(that is my interpretation of the task), 
    which in turn calls d1_wait_ack. 
    Given that I can not modify the d1_udp.h, which in turn do not allow me to add parameters to the functions, I instead manipulated
    peer->next_seqno by adding 2 to it and calling d1_send_data again after the first failed ACK. The next time d1_wait_ack is called,
    I check whether the seqno is larger than 2, and if is and the ACK fails I do not recall d1_send_data. Also I remember to set the 
    peer->next_seqno back to its original number, which should always be either 0 or 1. 
    In d1_send_data I set the ack in header to be equal to peer->next_seqno % 2, so that the seqno is correct for the second call of d1_send_data even though 
    peer->next_seqno is 2 or 3. 

    If output is: "Timeout: Resource temporarily unavailable"  "No data available, try again later." the recvfrom did not receive any ack,
    if it did receive another ack and it was wrong still the output is "Received wrong ACK". This is also reflected in the return value.

    Also I enable the socket timer before I call recvfrom, and immediately disable it so there is no chance I return any values before I can disable it.

    I also made a check for when it does not recieve an ACK which distinguishes between if the reason is a recvfrom error or if it is because there 
    was not sent a package in the 1 second timer.
    Returns 1 in case of success, in case of failure returns -1 in case of recvfrom = -1, or appropriate error in other case.


int  d1_recv_data (struct D1Peer* peer, char* buffer, size_t sz):
    If the caller is not interested in the source address, src_addr and addrlen should be specified as NULL.
    I can not see anywhere in the assignment that we are interested in the source address, since we know all the servers, however for scalability
    I am using recvfrom with source address, src_addr and addrlen = NULL, so that these values can be changed if you need the source address.
    As of now the recvfrom is equal to recv. Which I assume is sufficient for this task. 

    This function is pretty straight forward. I send an ACK-packet with the wrong ack if (header->size != bytes_received) and 
    if (calculated_checksum != header->checksum) to indicate that there has occurred an error. If everything is as it should I send correct ACK.
    If we assume that the server functions the same way as our program, resending the packet in case of wrong ACK, we could have called d1_recv_data
    again. However I think it is best to let the user decide how many calls of d1_recv_data they want to make in case of wrong checksum or mismatch
    in header size. Especially now when the user can differentiate between these due to return errors.
    In case of success return size of payload, in case of failure return appropriate error.


void d1_send_ack (struct D1Peer* peer, int seqno):
    I added a check to see if the seqno is legal or not, 1 or 0, because I tampered with the seqno in d1_wait_ack. 
--------------------------------------------------------------------------------------------------------------------------------------

D2:

    struct LocalTreeStore
    {
        int number_of_nodes;
        int currentNodes;
        struct NetNode** nodes;
    };

    I have modified the LocalTreeStore struct by adding an int which represents the amount current nodes in the tree.
    I also added an array of pointers to store the nodes in the tree structure. From what I can see in the test I assume that
    the nodes are always sent in increasing order meaning that the first node sent is root and has id = 0, the next id = 1, and so on. 
    Because of this a list is perfect to store nodes, due to the fact that nodes[0] always give node with id = 0, 
    and nodes[5] gives node with id = 5.

    In d2 I print less error-messages because most of the functions call d1-functions which in turn will show error messages.
    I have chosen not to create enums for errors in d2_add_to_local_tree and d2_print_node, but rather print the error directly, since 
    I find these error to be very specific for only these two methods.


D2Client* d2_client_create (const char* server_name, uint16_t server_port):
    Dynamically allocates memory for D2Client*, calls d1_create_client() to create and return a D1Peer* for D2Client->peer.
    In case of failure this function closes socket and frees memory depending on the cause of failure.
    Returns client* in case of success, appropriate error in case of failure.


D2Client* d2_client_delete (D2Client* client):
    Calls d1_create_client() for client->peer, which closes socket and frees peer. 
    Then frees client.
    Always returns NULL;


int d2_send_request (D2Client* client, uint32_t id):
    Creates a PacketRequest and sends it to the server. Uses d1_send_data() which in turn adds header, and receives and validates ack.
    I decided that if bytes_sendt is not equal to expected num of bytes this is a failure and we return ERR_SENDTO_NOT_MATCHING_EXPECTED;
    Returns bytes_sent in case of success, appropriate error in case of failure.


int d2_recv_response_size (D2Client* client):
    Calls d1_recv_data which received data and sends appropriate ACK in case of success and wrong ACK in case of failure.
    Function checks that we got expected bytes, in case of failure returns ERR_RECV_NOT_MATCHING_EXPECTED.
    The function checks if the packet is in fact a packetResponsSize, and returns ERR_NOT_PACKETRESPONSESIZE if not.
    Returns response->size in case of success, appropriate error in case of failure.


int d2_recv_response (D2Client* client, char* buffer, size_t sz):
    Calls d1_recv_data to get packet. If d1_recv_data == -1 we simply return ERR_RECV_NOT_MATCHING_EXPECTED, we do not print error because
    d1_recv_data does it for us.
    I check if bytes_received is larger or equal to sizeof(PacketResponse) to safely cast buffer to a packet repsons to 
    check if payload_size == expected size.
    Returns bytes_received in case of success, appropriate error in case of failure.


LocalTreeStore* d2_alloc_local_tree (int num_nodes):
    Dynamically allocates memory for tree-stucture. Initializes values to 0. And allocate a list of nodes for tree-structure.
    Initializes all the nodes to NULL.
    Returns tree-structure* in case of success, returns NULL in case of failure.


void d2_free_local_tree (LocalTreeStore* nodes): 
    Frees all the nodes in tree->nodes. Then frees tree->nodes. Then it frees the tree.
    In case of success it prints a message indicating success.


int d2_add_to_local_tree (LocalTreeStore* nodes, int node_idx, char* buffer, int buflen):
    Has a pointer to current position of buffer and to the end.
    Iterates through buffer, allocating memory for a netNode when needed, providing appropriate checks such as not reading out of buffer bounds.
    Set all node values, setting excess child[] space to 0. Move current buffer position to correct position, and adding node to tree-structure.
    Lastly updating nodeidx.
    Returns updated node_idx in case of success, and -1 with appropriate error-msg in case of failure.


void d2_print_node (LocalTreeStore* nodes_out, NetNode* netnode, char* indent):
    I created this help function to d2_print_tree. This function calls itself recursivly on all of the netNodes children.
    If node is not null it prints itself with the char* indent at the beginning. Then it allocates memory for a new string with 
    a new size 3 larger than the original indent. This is for adding "--" and a 0 byte. Then it calls d2_print_node on all the netNodes
    children, and lastly frees the memory for the new string.


void d2_print_tree (LocalTreeStore* nodes):
    Creates a char* indent = "" for the root node. Then calls d2_print_node with the root node.
--------------------------------------------------------------------------------------------------------------------------------------