/* ======================================================================
 * DO NOT MODIFY THIS FILE.
 * ====================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "d1_udp.h"

/* This programs tests your implementations of the d1 functions.
 * It creates an D1 association with a server running on the given host and port.
 *
 * Through this association, it sends the strings connect, ping (twice) and disconnect.
 * After sending each of the ping packets, it waits to receive a pong packet from
 * the server.
 *
 * Note that D1 send functions wait until they have received a suitable ACK packet
 * from the server. If they receive an ACK but it acknowledges the wrong sequence number
 * (0 instead of 1 or 1 instead of 0), they retransmit the data.
 * D1 receive functions send a suitable ACK to the server when they receive a packet.
 */

int main( int argc, char* argv[] )
{
    if( argc < 3 )
    {
        fprintf( stderr, "Usage %s <host> <port>\n"
                         "    <host> - name of the server. Can be localhost, a regular name, or an address in dotted decimal.\n"
                         "    <port> - UDP port the server uses for listening.\n"
                         "\n", argv[0] );
        return -1;
    }

    char*    server_name = argv[1];
    uint16_t server_port = atoi(argv[2] );

    /* Create a suitable data structure to manage the assocation of this client
     * with a server.
     */
    D1Peer*  client = d1_create_client( );
    if( ! client )
    {
        printf( "Failed to create D1 client.\n" );
        return -1;
    }

    printf( "Created D1 client\n" );

    int  ret;
    char buffer[1024];

    /* Discover the address information for the server we want to contact.
     * If the discovery succeeds, store the information in the D1Peer structure
     * client.
     * Otherwise, terminate the client and quit.
     */
    ret = d1_get_peer_info( client, server_name, server_port );
    if( ret == 0 )
    {
        printf( "Failed to resolve the name for %s:%d\n", server_name, server_port );
        d1_delete( client );
        return -1;
    }

    /* d1_send_data sends the string "connect" to the server, blocks until it receives and
     * ACK from the server. If it is the wrong ACK (e.g. the ACKNO flags is 1 although the
     * SEQNO field of our packet was 0), d1_send_data send the packet again.
     */
    char* buf = "connect";
    int   sz  = strlen(buf);
    ret = d1_send_data( client, buf, sz );
    if( ret < 0 )
    {
        d1_delete( client );
        return -1;
    }

    buf = "ping";
    sz  = strlen(buf);

    for( int i=0; i<2; i++ )
    {
        sleep(1);

        /* Send the string "ping". Behaviour as above.
         */
        ret = d1_send_data( client, buf, sz );
        if( ret < 0 )
        {
            d1_delete( client );
            return -1;
        }

        /* Receive a packet containing a string from the server.
         * When a packet arrives and it is type is FLAG_DATA, copy the value of its SEQNO
         * flag into the ACKNO flag of an ACK packet and send the ACK packet to the
         * server.
         */
        ret = d1_recv_data( client, buffer, 1000 );
        if( ret < 0 )
        {
            d1_delete( client );
            return -1;
        }

        buffer[ret] = 0;
        printf( "%d: Received >>>%s<<<\n", getpid(), buffer );
    }

    /* Send the string "disconnect". Behaviour as above. Expect that the server quits.
     */
    buf = "disconnect";
    sz  = strlen(buf);
    ret = d1_send_data( client, buf, sz );
    if( ret < 0 )
    {
        d1_delete( client );
        return -1;
    }

    /* Delete the D1 data structure and close the socket.
     */
    client = d1_delete( client );

    return 0;
}

