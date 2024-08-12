/* ======================================================================
 * DO NOT MODIFY THIS FILE.
 * ====================================================================== */

#ifndef D1_UDP_H
#define D1_UDP_H

#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "d1_udp_mod.h"

/* This is the packet header for the communication method that is implemented by the D1
 * entity.
 *
 * The maximum packet size if 1024 byte including the headers.
 *
 * The header precedes every packet that is sent over the network.
 * The header fields are encoded in network byte order when they are sent over the
 * network.
 * - flags comprises 16 bits with the following meaning:
 *   bit 15: if true (1), this is a data packet. If it is false (0), this is not a
 *           data packet.
 *   bit  8: if true, this is an ACK packet.
 *   bit  7: if this is a data packet of connect packet, this contains the sequence
 *           number, which is either 0 or 1. If this is a disconnect or ACK packet,
 *           this is 0.
 *   bit  0: if this is an ACK packet, this bit contains the sequence number that is
 *           acknowledged (ie. it is a response to a data or connect packet).
 *   bits 14-9 og 6-1: always 0
 *
 *   Only one of 8 or 15 can be 1.
 *   Only data packets can contain data.
 * - checksum is computed by computing bit-wise XOR for all the fields flags, the upper
 *   and lower half of the size field, and (in case of a data packet), the entire data
 *   of the packet. If the number of bytes in the data is an odd number, the data is
 *   padded with a 0-byte for the checksum computation. This padding byte is not transmitted.
 * - size contains the number of bytes in the packet. For connect, disconnect and ACK packets,
 *   it is always 8. For data packets, it counts the bytes of the header and the data.
 *
 * When the connection from a node ("client") to a direct neighbour ("server") is attempted,
 * the client send a connect packet. The client chooses the sequence number (0 or 1). The
 * connection succeeds if the server responds with an ACK packet containing the same sequence
 * number.
 *
 * When the connection is establish, both client and server can send data packets. It is NOT
 * REQUIRED to implement an D1 entity that can always receive packets. It is acceptable that
 * the D1 entity waits for packets only when the next higher layer expects data packet. But
 * whenever it receives a data packet, it must send the appropiate ACK. Furthermore, it D1
 * must always block after sending a data packet or connect packet until it has received the
 * correct ACK. If it receives the wrong ACK or does not receive an ACK within 1 second, it
 * must resend its data packet or connect packet.
 *
 * Disconnect packets and ACK packets are not ACKed by the receiving side.
 *
 * Note once more that a packet including the D1Header and the data can never be larger
 * than 1024 bytes.
 */
struct D1Header
{
    uint16_t flags;
    uint16_t checksum;
    uint32_t size;
};

typedef struct D1Header D1Header;

/* These are the possible values of D1Header.flags in host byte order.
 * When you send D1Headers over the network, they must be sent in network
 * byte order.
 */
#define FLAG_DATA       (1 << 15)
#define FLAG_ACK        (1 << 8)
#define SEQNO           (1 << 7)
#define ACKNO           (1 << 0)

/** Create a UDP socket that is not bound to any port yet. This is used as a
 *  client port.
 *  Returns the pointer to a structure on the heap in case of success or NULL
 *  in case of failure.
 */
D1Peer* d1_create_client( );

/** Take a pointer to a D1Peer struct. If it is non-NULL, close the socket
 *  and free the memory of the server struct.
 *  The return value is always NULL.
 */
D1Peer* d1_delete( D1Peer* peer );

/** Determine the socket address structure that belongs to the server's name
 *  and port. The server's name may be given as a hostname, or it may be given
 *  in dotted decimal format.
 *  In this excersize, you do not support IPv6. There are no extra points for
 *  supporting IPv6.
 *  Returns 0 in case of error and 1 in case of success.
 */
int d1_get_peer_info( struct D1Peer* client, const char* servername, uint16_t server_port );

/** If the buffer does not exceed the packet size, the function adds the D1 header and sends
 *  it to the peer.
 *  Returns the number of bytes sent in case of success, and a negative value in case
 *  of error.
 */
int  d1_send_data( struct D1Peer* peer, char* buffer, size_t sz );

/** Called by d1_send_data when it has successfully sent its packet. This functions waits for
 *  an ACK from the peer.
 *  If the sequence number of the ACK matches the next_seqno value in the D1Peer stucture,
 *  this function changes the next_seqno in the D1Peer data structure
 *  (0->1 or 1->0) and returns to the caller.
 *  If the sequence number does not match, d1_send_data followed by d1_wait_ack is called
 *  again.
 *  Returns a positive value in case of success, and a negative value in case of error.
 *
 *  This function is only meant to be called by d1_send_data. You don't have to implement it.
 */
int  d1_wait_ack( D1Peer* peer, char* buffer, size_t sz );

/** Call this to wait for a single packet from the peer. The function checks if the
 *  size indicated in the header is correct and if the checksum is correct.
 *  In case of success, remove the D1 header and store only the payload in the buffer
 *  that is passed a parameter. The size sz is the size of the buffer provided by the
 *  caller.
 *  If size or checksum are incorrect, an ACK with the opposite value is sent (this should
 *  trigger the sender to retransmit).
 *  In other cases, an error message is printed and a negative number is returned to the
 *  calling function.
 *  Returns the number of bytes received as payload in case of success, and a negative value
 *  in case of error. Empty data packets are allowed, and a return value of 0 is therefore
 *  valid.
 */
int  d1_recv_data( struct D1Peer* peer, char* buffer, size_t sz );

/** Send an ACK for the given sequence number.
 */
void d1_send_ack( struct D1Peer* peer, int seqno );

#endif /* D1_UDP_H */

