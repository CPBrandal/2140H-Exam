/* ======================================================================
 * YOU ARE EXPECTED TO MODIFY THIS FILE.
 * ====================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <limits.h>
#include <sys/time.h>
#include <errno.h>
#include "d1_udp.h"


void errorToString( ErrorCode code ) 
{
    switch (code) {
        case ERR_INVALID_ARGUMENT:
            fprintf(stderr, "Invalid argument sent to function\n");
            break;
        case ERR_COULD_NOT_GET_IP:
            fprintf(stderr, "Unable to get IP from hostname or dotted decimal\n");
            break;
        case ERR_RECV_NOT_MATCHING_EXPECTED:
            fprintf(stderr, "Actual received bytes does not match expected received bytes\n");
            break;
        case ERR_SENDTO_NOT_MATCHING_EXPECTED:
            fprintf(stderr, "Actual sent bytes does not match expected sent bytes\n");
            break;
        case ERR_WRONG_ACK_RECEIVED:
            fprintf(stderr, "Received wrong ACK\n");
            break;
        case ERR_WRONG_CHECKSUM:
            fprintf(stderr, "Checksum does not match expected checksum\n");
            break;
        case ERR_INVALID_SEQNO:
            fprintf(stderr, "Invalid sequence number\n");
            break;
        case ERR_ACK_SEND_FAILED:
            fprintf(stderr, "ACK send failed\n");
            break;
        case ERR_HEADER_SIZE_MISMATCH:
            fprintf(stderr, "Given header size does not match size received\n Sending wrong ACK\n");
            break;
        case ERR_NOT_PACKETRESPONSESIZE:
            fprintf(stderr, "Packet received was not of type packet-respons-size\n");
            break;
        default:
            fprintf(stderr, "An unknown error occurred\n");
    }
}

uint16_t make_checksum( D1Header* header, char* buffer, size_t sz ) 
{
    // Set checksum to 0 to be sure that the xor will be correct
    uint16_t checksum = 0; 

    // Perform xor oepration on flags
    checksum ^= header->flags;

    // Perform xor operation on size, in two parts given the size is 32 bits
    uint32_t size = header->size;
    checksum ^= (size >> 16) & 0xFFFF;  // Higher 16 bits
    checksum ^= size & 0xFFFF;          // Lower 16 bits

    // If buffer or sz is NULL or 0, we assume that this is an ack-packet, which does not have a payload, and we return the checksum
    if(!buffer || sz == 0){
        return checksum;
    }
    
    // Perform xor operation on the buffer for sz/2 times
    for (size_t i = 0; i < sz / 2; i++) {
        // Properly combine two consecutive bytes to form a 16-bit number
        uint16_t twoBytes = ((uint16_t)buffer[2 * i] << 8) | (uint8_t)buffer[2 * i + 1];
        checksum ^= twoBytes;
    }

    if (sz % 2 != 0) {
        uint16_t lastByte = (uint16_t)buffer[sz - 1] << 8;  // Shift left if padding to MSB is intended
        checksum ^= lastByte;
    }
    return checksum;
}

// Create a socket for UDP with ipv4, 0 flag
int create_socket( ) 
{ 
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1){
        perror("Socket");
        return -1;
    }
    return sockfd;
}

// Toggle a timer for the socket, used for d1_wait_ack
int toggle_socket_timer( int socket, int enable, int sec, long usec ) 
{
    struct timeval timeout;
    timeout.tv_sec = enable ? sec : 0;
    timeout.tv_usec = enable ? usec : 0;

    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Setsockopt failed to set/clear timeout");
        return -1;
    }
    return 0;
}

D1Peer* d1_create_client( )
{
    D1Peer* client = malloc(sizeof(struct D1Peer)); // Allocate memory for client
    if(!client){
        perror("malloc");
        return NULL;
    }

    int socketfd = create_socket();
    if(socketfd == -1){ // If socket fails, we must free the memory allocated for client
        free(client);
        return NULL;
    }

    // Initialize client->socket, set rest to 0
    client->socket = socketfd;
    memset(&client->addr, 0, sizeof(client->addr));
    client->next_seqno = 0;

    return client;
}

D1Peer* d1_delete( D1Peer* peer )
{
    // If peer is not NULL first close socket then free allocated memory for peer
    if(peer){
        close(peer->socket);
        free(peer);
        printf("Successfully deleted client\n");
    }
    return NULL;
}

int d1_get_peer_info( struct D1Peer* client, const char* servername, uint16_t server_port )
{
    // If client or servername is null return -1 
    if (!client || !servername || server_port < 1024){
        //fprintf(stderr, "Invalid parameter for d1_get_peer_info\n");
        errorToString(ERR_INVALID_ARGUMENT);
        return 0;
    }

    // Initialize clients addr (sockaddr_in)
    client->addr.sin_family = AF_INET;
    client->addr.sin_port = htons(server_port);
    struct in_addr ip_addr;
    memset(&ip_addr, 0, sizeof(ip_addr)); // Set all values to 0

    // Get ip from dotted decimal
    int ip = inet_pton(AF_INET, servername, &ip_addr.s_addr);
    if(ip == 1){
        client->addr.sin_addr = ip_addr; // Set client->addr.sin_addr in case of dotted decimal
        return 1;
    }
    // Get ip from hostname
    struct hostent *hostN = gethostbyname(servername);
    if (hostN != NULL) {
        memcpy(&(client->addr.sin_addr.s_addr), hostN->h_addr, hostN->h_length); // Set client->addr.sin_addr in case of hostname
        return 1;
    }

    errorToString(ERR_COULD_NOT_GET_IP);
    return 0; // Return 0 in case we could not convert servername from dotted decimal or hostname
}

int d1_send_data( D1Peer* peer, char* buffer, size_t sz )
{
    // If peer or buffer is null, or if size is larger than 1016 (max size - header)
    if (peer == NULL || buffer == NULL || sz > 1016) {
        //fprintf(stderr, "Invalid parameter for d1_send_data\n");
        errorToString(ERR_INVALID_ARGUMENT);
        return ERR_INVALID_ARGUMENT;
    }

    size_t totSize = sz + sizeof(D1Header);
    char packet[totSize];

    // Initialize header, set FLAG_DATA, bit 7 to be equal to peer->next_seqno % 2, set size and checksum.
    D1Header header;
    header.flags = 0;
    header.flags |= FLAG_DATA;
    header.flags |= (peer->next_seqno % 2 << 7);
    header.size = 0;
    header.size = totSize;
    header.checksum = 0;
    header.checksum = make_checksum(&header, buffer, sz);

    // Convert header variables to network byte order
    header.flags = htons(header.flags);
    header.size = htonl(header.size);
    header.checksum = htons(header.checksum);
    
    memcpy(packet, &header, sizeof(D1Header)); // Copy header into packet
    memcpy(packet + sizeof(D1Header), buffer, sz); // Copy buffer into packet with offset sizeof(D1Header)

    int bytes_sent = sendto(peer->socket, 
                                packet, 
                                totSize, 
                                0,
                                (struct sockaddr*)&peer->addr, 
                                sizeof(peer->addr));
    // Check if sendto failed
    if(bytes_sent == -1){
        perror("sendto");
        return -1;
    }
    // Check if bytes sent matches with what we expect
    if((size_t)bytes_sent != totSize){
        fprintf(stderr, "Expected bytes sent: %li, actual bytes sent: %d\n", totSize, bytes_sent);
        errorToString(ERR_RECV_NOT_MATCHING_EXPECTED);
        return ERR_RECV_NOT_MATCHING_EXPECTED;
    }   

    int ack = d1_wait_ack(peer, buffer, sizeof(D1Header));
    if(ack == -1){
        return -1;
    }

    return bytes_sent;
}

int d1_wait_ack( D1Peer* peer, char* buffer, size_t sz ) 
{
    if (!peer) {
        //fprintf(stderr, "Invalid input for d1_wait_ack, D1Peer is null\n");
        errorToString(ERR_INVALID_ARGUMENT);
        return ERR_INVALID_ARGUMENT;
    }
    int seqno = peer->next_seqno; // Storing seqno

    // Checking if resend has been done
    if(seqno > 1){
        peer->next_seqno -= 2; // Set peer->next_seqno back to its original number
    }

    // Enable timer on the socket
    if(toggle_socket_timer(peer->socket, 1, 1, 0) == -1) {
        fprintf(stderr, "Failed to enable timeout\n");
    }

    D1Header header;
    int bytes_received = recvfrom(peer->socket, &header, sz, 0, NULL, NULL);

    // Disable timer on socket since socket-timer is only demanded in d1_wait_ack
    if (toggle_socket_timer(peer->socket, 0, 0, 0) == -1) {
        fprintf(stderr, "Failed to disable timeout\n");
    }

    if(bytes_received == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) { // Check if fail-cause is related to recv or not
            perror("Timeout");
            printf("No data available, try again later.\n");
            return -1;
        } else { 
            perror("Recv failed\n");
            return -1;
        }
    } 

    if (bytes_received != sizeof(D1Header)) { // Check if expected 
        fprintf(stderr, "Bytes recieved does not match expected bytes\n");
        errorToString(ERR_RECV_NOT_MATCHING_EXPECTED);
        return ERR_RECV_NOT_MATCHING_EXPECTED;
    }

    // Convert header.flags to host byte order
    header.flags = ntohs(header.flags);

    int recievedSeqno = (int)(header.flags & 1); //Store ack seqno

    // Check if seqno received is correct
    if (recievedSeqno == peer->next_seqno) {
        peer->next_seqno ^= 1;  // Toggle the expected sequence number for next time in case of success
        return 1;
    } else {
        if(seqno > 1){ // Still wrong ack on the resend
            errorToString(ERR_WRONG_ACK_RECEIVED);
            return ERR_WRONG_ACK_RECEIVED;
        } else {
            errorToString(ERR_WRONG_ACK_RECEIVED);
            printf("Resending packet\n");
            peer->next_seqno += 2; // Manipulate peer->next_seqno to only do one resend
            return d1_send_data(peer, buffer, sz); // Resending packet
        }
    }
}


int d1_recv_data( struct D1Peer* peer, char* buffer, size_t sz )
{
    if(peer == NULL || sz > 1024 || sz < 8 || buffer == NULL){
        //fprintf(stderr, "Could not recieve data, illegal argument(s)\n");
        errorToString(ERR_INVALID_ARGUMENT);
        return ERR_INVALID_ARGUMENT;
    }

    char tempBuffer[sz]; // Initialize temporary buffer with max size

    int bytes_received = recvfrom(peer->socket, tempBuffer, sizeof(tempBuffer), 0, NULL, NULL);
    if (bytes_received == -1) {
        perror("Recv failed");
        return -1;
    }

    D1Header* header = (D1Header*)tempBuffer; // Cast temporary buffer to header
    // Convert to host byte
    header->flags = ntohs(header->flags);
    header->checksum = ntohs(header->checksum);
    header->size = ntohl(header->size);

    // If header->size is different from bytes_received send wrong ack, return -1
    if((int)header->size != bytes_received){ 
        errorToString(ERR_HEADER_SIZE_MISMATCH);
        d1_send_ack(peer, peer->next_seqno ^ 1);
        return ERR_HEADER_SIZE_MISMATCH;
    }

    // Calculate checksum
    uint16_t calculated_checksum = make_checksum(header, tempBuffer + sizeof(D1Header), bytes_received - sizeof(D1Header));
    if(calculated_checksum != header->checksum){ // Compare calculated checksum to received checksum
        //fprintf(stderr, "Invalid checksum: recieved %u, calculated %u\n", header->checksum, calculated_checksum);
        errorToString(ERR_WRONG_CHECKSUM);
        d1_send_ack(peer, peer->next_seqno ^ 1); // If checksum is different from calculated checksum send wrong ack, return -1
        return ERR_WRONG_CHECKSUM;
    }

    memcpy(buffer, tempBuffer + sizeof(D1Header), bytes_received - sizeof(D1Header)); // Buffer containing only payload

    int seqno = (header->flags & SEQNO) >> 7;
    d1_send_ack(peer, seqno);

    return bytes_received-sizeof(D1Header);
}


void d1_send_ack( struct D1Peer* peer, int seqno )
{
    if (peer == NULL){
        errorToString(ERR_INVALID_ARGUMENT); 
        return;
    }
    if(seqno < 0 || seqno > 1){
        errorToString(ERR_INVALID_SEQNO);
        return;
    }

    D1Header header;
    header.flags = 0;
    header.flags = FLAG_ACK; // Set bit representing ack to 1
    header.flags |= (seqno << 0); // Ack-seqno-bit == seqno
    header.size = (sizeof(D1Header)); 
    header.checksum = make_checksum(&header, NULL, 0); 

    // Convert header variables to network byte order
    header.flags = htons(header.flags);
    header.checksum = htons(header.checksum);
    header.size = htonl(header.size);

    // Initialize a buffer with size of D1Header
    char buf[sizeof(D1Header)];
    memcpy(buf, &header, sizeof(D1Header)); // Copy header into buffer

    int bytes_sent = sendto(peer->socket, buf, sizeof(D1Header), 0,
                                (struct sockaddr*)&peer->addr, sizeof(peer->addr));
    if(bytes_sent == -1){
        perror("Sendto");
        return;
    }
    if(bytes_sent != (int)sizeof(D1Header)){ 
        errorToString(ERR_SENDTO_NOT_MATCHING_EXPECTED);
    }
}