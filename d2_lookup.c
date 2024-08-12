/* ======================================================================
 * YOU ARE EXPECTED TO MODIFY THIS FILE.
 * ====================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "d2_lookup.h"


D2Client* d2_client_create( const char* server_name, uint16_t server_port )
{
    // Dynamically allocate memory for D2Client
    D2Client* client = malloc(sizeof(D2Client));
    if(!client){
        perror("Failed to allocate memory for peer\n");
        return NULL;
    }

    client->peer = d1_create_client(); // Create new client for D2Client
    if(client->peer == NULL){
        free(client); // Free allocated memory in case of failure
        return NULL;
    }
    int getInfo = d1_get_peer_info(client->peer, server_name, server_port);
    if(getInfo == -1){
        d2_client_delete(client); // Function frees memory in case of failure
        return NULL;
    } else {
        return client;
    }
}

D2Client* d2_client_delete( D2Client* client )
{
    if(client){
        d1_delete(client->peer); // Closes socket and frees(client->peer)
        free(client); // Free allocated memory for client
    }

    return NULL;
}

int d2_send_request( D2Client* client, uint32_t id )
{
    if(!client || id < 1000){
        errorToString(ERR_INVALID_ARGUMENT);
        return ERR_INVALID_ARGUMENT;
    }

    // Set packet size
    char packet[sizeof(PacketRequest)]; 

    // Initialize PacketRequest, set all bits to 0
    PacketRequest packetRequest;
    memset(&packetRequest, 0, sizeof(PacketRequest));

    // Convert from host- to network byte order
    packetRequest.type = htons(TYPE_REQUEST);
    packetRequest.id = htonl(id);

    // Copy packetRequest into packet
    memcpy(packet, &packetRequest, sizeof(PacketRequest));

    int bytes_sent = d1_send_data(client->peer, packet, sizeof(PacketRequest));

    if(bytes_sent <= 0){
        return ERR_SENDTO_NOT_MATCHING_EXPECTED;
    }

    if(bytes_sent != sizeof(PacketRequest)+sizeof(D1Header)){
        errorToString(ERR_SENDTO_NOT_MATCHING_EXPECTED);
        return ERR_SENDTO_NOT_MATCHING_EXPECTED;
    }

    return bytes_sent;
}

int d2_recv_response_size( D2Client* client )
{
    if(!client){
        errorToString(ERR_INVALID_ARGUMENT);
        return ERR_INVALID_ARGUMENT;
    }
    
    size_t sz = sizeof(D1Header)+sizeof(PacketResponseSize);
    char buffer[sz];

    int bytes_recieved = d1_recv_data(client->peer, buffer, sz);
    if(bytes_recieved == -1){
        return ERR_RECV_NOT_MATCHING_EXPECTED;
    }

    if(bytes_recieved != (int)(sz-sizeof(D1Header))){
        errorToString(ERR_RECV_NOT_MATCHING_EXPECTED);
        return ERR_RECV_NOT_MATCHING_EXPECTED;  
    }   

    // Cast buffer to packetHeader
    PacketHeader* packetHeader = (PacketHeader*)buffer;

    if(ntohs(packetHeader->type) != TYPE_RESPONSE_SIZE){ // Check that we in fact recieved a PacketResponseSize
        errorToString(ERR_NOT_PACKETRESPONSESIZE);
        return ERR_NOT_PACKETRESPONSESIZE;
    }

    // Cast buffer to PacketResponsSize to extract packet->size
    PacketResponseSize* packet = (PacketResponseSize*)buffer;
    packet->size = ntohs(packet->size);
    return packet->size;
}

int d2_recv_response( D2Client* client, char* buffer, size_t sz )
{
    if(!client || buffer == NULL || sz < 1024) { // Client or buffer cant be NULL, the caller must provide a buffer where up to 1024 bytes can be stored.
        errorToString(ERR_INVALID_ARGUMENT);
        return ERR_INVALID_ARGUMENT;
    }

    int bytes_received = d1_recv_data(client->peer, buffer, sz);

    if(bytes_received == -1){
        return ERR_RECV_NOT_MATCHING_EXPECTED;
    }

    // Check if we can safetly cast buffer to packet-response
    if(bytes_received >= (int)sizeof(PacketResponse)){ 
        PacketResponse* packetResponse = (PacketResponse*)buffer;
        int expected_bytes = ntohs(packetResponse->payload_size);
        if(bytes_received != expected_bytes){ // Check if package was the same as payload->size
            errorToString(ERR_RECV_NOT_MATCHING_EXPECTED);
            return ERR_RECV_NOT_MATCHING_EXPECTED;
        }
    } else {
        errorToString(ERR_RECV_NOT_MATCHING_EXPECTED);
        return ERR_RECV_NOT_MATCHING_EXPECTED;
    }
    return bytes_received;
}

LocalTreeStore* d2_alloc_local_tree( int num_nodes )
{
    // Allocates memory for tree structure
    LocalTreeStore* treeStructure = malloc(sizeof(LocalTreeStore));
    if(!treeStructure){
        perror("Memory allocation failed for LocalTreeStore");
        return NULL;
    }

    // Initialize treestructure
    treeStructure->number_of_nodes = num_nodes;
    treeStructure->currentNodes = 0;
    treeStructure->nodes = malloc(sizeof(NetNode*) * num_nodes); // Allocate memory for list of nodes

    if(!treeStructure->nodes){
        perror("Memory allocation failed for LocalTreeStore->nodes");
        free(treeStructure);
        return NULL;
    }

    // Initialize all the netnodes to null
    for(int i = 0; i < num_nodes; i++){
        treeStructure->nodes[i] = NULL;
    }

    return treeStructure;
}

void d2_free_local_tree( LocalTreeStore* nodes )
{
    // Free all nodes, then free allocated memory for storing nodes, lastly free the whole tree
    if(nodes){
        for(int i = 0; i < nodes->number_of_nodes; i++){
            free(nodes->nodes[i]);
        }
        free(nodes->nodes);
        free(nodes);
        printf("Tree successfully deleted\n");
    }
}

int d2_add_to_local_tree( LocalTreeStore* nodes_out, int node_idx, char* buffer, int buflen )
{
    if(!nodes_out || !buffer || buflen < 0){
        errorToString(ERR_INVALID_ARGUMENT);
        return ERR_INVALID_ARGUMENT;
    }

    char* currentBufferPosition = buffer;
    char* bufferEnd = buffer + buflen;  // Point to end of the buffer to check max boundry

    while (currentBufferPosition < bufferEnd) {
        if (currentBufferPosition + sizeof(uint32_t) * 3 > bufferEnd) { // Check that we do not exceed the buffer, need space for minimum length of a node
            fprintf(stderr, "Buffer overflow when accessing basic node data\n");
            return -1;
        }

        // Allocate memory for the new node
        NetNode* newNode = (NetNode*)malloc(sizeof(NetNode));
        if (!newNode) {
            perror("Memory allocation failed for NetNode");
            return -1;
        }

        // Read id, value, num_children directly
        newNode->id = ntohl(*(uint32_t*)currentBufferPosition);
        newNode->value = ntohl(*(uint32_t*)(currentBufferPosition + 4));
        newNode->num_children = ntohl(*(uint32_t*)(currentBufferPosition + 8));

        // Check that nodes num_children does not exceed legal limit of 5
        if (newNode->num_children > 5) {
            fprintf(stderr, "Node has invalid number of children: %u\n", newNode->num_children);
            free(newNode);
            return -1;
        }

        currentBufferPosition += 12; // Move buffer-position past id, value and num_children

        // Check that we have enough data in buffer to access all children 
        if (currentBufferPosition + sizeof(uint32_t) * newNode->num_children > bufferEnd) {
            fprintf(stderr, "Buffer overflow when accessing child IDs\n");
            free(newNode);
            return -1;
        }

        // For each child, convert to host-byte order, and store id in newNode->child_id
        for (int i = 0; i < (int)newNode->num_children; ++i) {
            newNode->child_id[i] = ntohl(*(uint32_t*)(currentBufferPosition + i * 4));
        }

        // Set all unused child_id[] to 0
        for (int i = (int)newNode->num_children; i < 5; ++i) {
            newNode->child_id[i] = 0; 
        }

        // Move buffer-position past the child-ids
        currentBufferPosition += sizeof(uint32_t) * newNode->num_children;

        // Store the node in the tree structure
        nodes_out->nodes[node_idx] = newNode;
        nodes_out->currentNodes++;

        if (node_idx >= nodes_out->number_of_nodes) { // Ensure we do not overflow the allocated node pointers
            fprintf(stderr, "Num of nodes exceeds the intended number\n");
            return -1;
        }
        node_idx++; // Shift node index
    }

    return node_idx;
}

void d2_print_node (LocalTreeStore* nodes_out, NetNode* netnode, char* indent) 
{
    if(netnode){
        // Print appropriate information about node
        printf("%sid %u value %u children %u\n", indent, netnode->id, netnode->value, netnode->num_children); 

        size_t new_indent_len = strlen(indent) + 3;  // Add space for "--" and a null byte
        char* new_indent = malloc(new_indent_len); // Allocate memory for the string
        if(new_indent){
            int cpy = sprintf(new_indent, "%s--", indent); // Write to new indent
            if(cpy < 0){
                fprintf(stderr, "Error with (sprintf) setting indent for child-node.\n"); // Dont need to exit function as this is only visual.
            }
            // Recursivly calls print for all child-nodes starting from the left(BFS)
            for(int i = 0; i < (int)netnode->num_children; i++){
                d2_print_node(nodes_out, nodes_out->nodes[netnode->child_id[i]], new_indent);
            }
            free(new_indent); // Free indent
        } else {
            perror("Memory allocation failed for indent");
        }
    }
}

void d2_print_tree( LocalTreeStore* nodes_out )
{
    if(nodes_out){
        char* initial_indent = "";
        d2_print_node(nodes_out, nodes_out->nodes[0], initial_indent);
    } else {
        errorToString(ERR_INVALID_ARGUMENT);
    }
}