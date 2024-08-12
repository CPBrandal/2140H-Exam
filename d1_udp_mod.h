/* ======================================================================
 * YOU CAN MODIFY THIS FILE.
 * ====================================================================== */

#ifndef D1_UDP_MOD_H
#define D1_UDP_MOD_H

#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* This structure keeps all information about this client's association
 * with the server in one place.
 * It is expected that d1_create_client() allocates such a D1Peer object
 * dynamically, and that d1_delete() frees it.
 */
struct D1Peer
{
    int32_t            socket;      /* the peer's UDP socket */
    struct sockaddr_in addr;        /* addr of my peer, initialized to zero */
    int                next_seqno;  /* either 0 or 1, initialized to zero */
};

typedef struct D1Peer D1Peer;

typedef enum Error{
    ERR_INVALID_ARGUMENT = -2, // Invalid argument for function/method
    ERR_COULD_NOT_GET_IP = -3, // Unable to get correct ip when trying to convert from dotted decimal and hostname
    ERR_RECV_NOT_MATCHING_EXPECTED = -4,    // Bytes received does not match expected amount
    ERR_SENDTO_NOT_MATCHING_EXPECTED = -5, // Bytes sent does not match expected amount
    ERR_WRONG_ACK_RECEIVED = -6 , // Received wrong ack
    ERR_WRONG_CHECKSUM = -7,  // Received wrong checksum
    ERR_INVALID_SEQNO = -8,     // Sequence number is invalid
    ERR_ACK_SEND_FAILED = -9, // Sending ack failed ack-size deviates from expected size 
    ERR_HEADER_SIZE_MISMATCH = -10, // Size spesified by packet header does not match received bytes
    ERR_NOT_PACKETRESPONSESIZE = -11 // Packet received was not of type Packet-Response-Size
} ErrorCode;

void errorToString( ErrorCode code );

#endif /* D1_UDP_MOD_H */