/* ======================================================================
 * DO NOT MODIFY THIS FILE.
 * ====================================================================== */

#ifndef D2_LOOKUP_H
#define D2_LOOKUP_H

#include "d2_lookup_mod.h"

/* At this layer, only the following types of packets are known:
 */
#define TYPE_REQUEST       (1 << 0) /* type is PacketRequest */
#define TYPE_RESPONSE_SIZE (1 << 1) /* type is PacketResponseSize */
#define TYPE_RESPONSE      (1 << 2) /* type is PacketResponse */
#define TYPE_LAST_RESPONSE (1 << 3) /* type is PacketResponse */

/* Every packet that is transmitted at this layer starts with a type.
 * An arriving buffer can be typecast to PacketHeader first to understand
 * the type of the arriving packet.
 *
 * The only use for this header is to interpret the type field to decide
 * which other packet type the arriving buffer should be typecast to.

 * type is in network byte order.
 */
struct PacketHeader
{
    uint16_t type;
};
typedef struct PacketHeader       PacketHeader;

/* PacketRequest packets are sent from the client to the server.
 * They contain their type and an ID, which must be >1000.
 *
 * Sending a PacketRequest to the server triggers a multi-packet
 * response. The server answer with PacketResponseSize and PacketResponse
 * packets to transfer tree-structured values that are unique for the
 * given ID.
 *
 * type and id are in network byte order.
 *
 * PacketRequest is always sizeof(PacketRequest) bytes long.
 */
struct PacketRequest
{
    uint16_t type;
    uint32_t id;
};
typedef struct PacketRequest      PacketRequest;

/* PacketResponseSize packets are sent from the server to the client.
 * They contain a size field that indicates how many NetNode structures
 * the following PacketReponse packets will contain.
 *
 * type and size are in network byte order.
 *
 * PacketResponseSize is always sizeof(PacketResponseSize) bytes long.
 */
struct PacketResponseSize
{
    uint16_t type;
    uint16_t size;
};
typedef struct PacketResponseSize PacketResponseSize;

/* PacketResponse packets are sent from the server to the client.
 * They contain the type field, which can be TYPE_RESPONSE or
 * TYPE_LAST_RESPONSE. If the type field is TYPE_RESPONSE, the
 * client can expect more PacketResponse packets, if it is
 * TYPE_LAST_RESPONSE, the server will not send more PacketResponse
 * packets.
 *
 * These packets contain the payload_size field that indicates the number of
 * bytes contained in this message.
 *
 * One or more abbreviated (!!!) NetNode structures follow the header,
 * but never more than 5. It is OK to assume that only TYPE_LAST_RESPONSE
 * packets have fewer than 5 NetNode structures following the header.
 *
 * NetNode structures are abbreviated, meaning that child_id[x] fields
 * are not transmitted over the network if x>=num_children.
 *
 * type, payload_size, and all fields of the NetNode structures are in
 * network byte order.
 *
 * PacketResponse is always PacketResponse.payload bytes long.
 */
struct PacketResponse
{
    uint16_t type;
    uint16_t payload_size;
};
typedef struct PacketResponse     PacketResponse;

/* NetNode structures are nodes of a tree. The root of the tree does always
 * have the id==0, and the other nodes have ids that are assigned in depth
 * first search order.
 *
 * For example, if a NetNode has id 6, num_children 2, child_id[0]==7 and
 * child_id[1]==12, this means that this NetNode is the parent of the
 * NetNodes with ids 7 and 12 in the tree. It also means that there are
 * 4 additional nodes lower in the tree as children or grandchildren of
 * NetNode 7.
 *
 * For transfer over the network, all fields of NetNode structure are in
 * network byte order.
 */
struct NetNode
{
    uint32_t id;
    uint32_t value;
    uint32_t num_children;
    uint32_t child_id[5];
};
typedef struct NetNode NetNode;

/* Create the information that is required to use the server with the given
 * name and port. Use D1 functions for this. Store the relevant information
 * in the D2Client structure that is allocated on the heap.
 * Returns NULL in case of failure.
 */
D2Client* d2_client_create( const char* server_name, uint16_t server_port );

/* Delete the information and state required to communicate with the server.
 * Returns always NULL.
 */
D2Client* d2_client_delete( D2Client* client );

/* Send a PacketRequest with the given id to the server identified by client.
 * The parameter id is given in host byte order.
 * Returns a value <= 0 in case of failure and positive value otherwise.
 */
int d2_send_request( D2Client* client, uint32_t id );

/* Wait for a PacketResponseSize packet from the server.
 *
 * Returns the number of NetNodes that will follow in several PacketReponse
 * packet in case of success. Note that this is _not_ the number of
 * PacketResponse packets that will follow because of PacketReponse can
 * contain several NetNodes.
 *
 * Returns a value <= 0 in case of failure.
 */
int d2_recv_response_size( D2Client* client );

/* Wait for a PacketResponse packet from the server.
 * The caller must provide a buffer where up to 1024 bytes can be stored.
 *
 * Returns the number of bytes received in case of success.
 * The PacketResponse header and all NetNodes included in the packet
 * are stored in the buffer.
 *
 * Returns a value <= 0 in case of failure.
 */
int d2_recv_response( D2Client* client, char* buffer, size_t sz );

/* Allocate one more more local structures that are suitable to receive num_nodes
 * tree nodes containing all the information from a NetNode.
 * It is expected that the memory for these structures is allocated dynamically.
 *
 * Change the LocalTreeStore to suit your need.
 *
 * Returns NULL in case of failure.
 */
LocalTreeStore* d2_alloc_local_tree( int num_nodes );

/* Release all memory that has been allocated for the local tree structures.
 */
void  d2_free_local_tree( LocalTreeStore* nodes ); 

/* Take the buffer that has been filled by d2_recv_response, and the buflen,
 * which is the number of bytes containing all the NetNodes stored in the buffer,
 * and add one node to the local tree for every NetNode in the buffer.
 *
 * One buffer can contain up to 5 NetNodes.
 *
 * node_idx is expected to be 0-based index of the tree nodes that are already
 * in the LocalTreeStore. The return value is node_idx increased by the additional
 * number of nodes that have been added to the LocalTreeStore.
 *
 * Tip: this may be helpful for array-based LocalTreeStores.
 *
 * Returns negative values in case of failure.
 */
int   d2_add_to_local_tree( LocalTreeStore* nodes, int node_idx, char* buffer, int buflen );

/* Print the current node in the LocalTreeStore to standard output
 * (e.g. using printf) in the following manner:
 *
id 0 value 72342 children 2
-- id 1 value 829347 children 1
---- id 2 value 298347 children 0
-- id 3 value 190238 children 2
---- id 4 value 2138 children 1
------ id 5 value 901293 children 0
---- id 6 value 128723 children 0
 *
 * In this printout, the tree's node 0 has two children, 1 and 3.
 * Node 1 has one child, 2.
 * Node 3 has two children, 4 and 6.
 * Node 4 has one child, 5.
 */
void  d2_print_tree( LocalTreeStore* nodes );

#endif /* D2_LOOKUP_H */

