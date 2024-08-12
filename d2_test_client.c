/* ======================================================================
 * DO NOT MODIFY THIS FILE.
 * ====================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "d2_lookup.h"

int main( int argc, char* argv[] )
{
    if( argc < 4 )
    {
        fprintf( stderr, "Usage %s <name> <port>\n"
                         "    <name> - hostname of the server, dotted decimal of string.\n"
                         "    <port> - UDP port the server uses for listening (integer).\n"
                         "    <id>   - an ID to look up a unique tree (integer, must be >1000).\n"
                         "\n", argv[0] );
        return -1;
    }

    const char* server_name = argv[1];
    uint16_t    server_port = atoi(argv[2] );
    uint32_t    student_id  = atoi( argv[3] );

    if( student_id < 1000 )
    {
        printf( "%d: D2 student ID %d is too low and not accepted.\n", getpid(), student_id );
        return -1;
    }

    /* Establish the required data to communicate with the server using
     * the server_name and server_port.
     */
    D2Client* client = d2_client_create( server_name, server_port );
    if( ! client )
    {
        printf( "Failed to create Lookup client.\n" );
        return -1;
    }

    printf( "%d: D2 created Lookup client\n", getpid() );

    /* Send the request to the server, passing only the <id> that was the
     * command parameter for this program.
     */
    int ret = d2_send_request( client, student_id );
    if( ret == 0 )
    {
        fprintf( stderr, "%d: Failed to send request to server\n", getpid() );
        return -1;
    }

    printf( "%d: D2 send request to look up id %d\n", getpid(), student_id );

    /* Receive a PacketResponseSize packet from the server.
     * If ret is positive, it provide the number of NetNodes that the server
     * is going to send.
     */
    ret = d2_recv_response_size( client );
    if( ret < 0 )
    {
        fprintf( stderr, "%d: Did not receive a response size\n", getpid() );
        return -1;
    }

    int num_nodes = ret;
    printf( "%d: D2 received response size %d\n", getpid(), num_nodes );

    /* Allocate memory to store the tree structures that will be arriving
     * from the server in local memory.
     */
    LocalTreeStore* store = d2_alloc_local_tree( num_nodes );

    int repeat = 1;
    int node_index = 0;
    while( repeat )
    {
        char buffer[1024];

        /* Receive a PacketResponse from the server.
         */
        int ret = d2_recv_response( client, buffer, 1024 );
        printf( "%d: D2 received a response packet\n", getpid() );

        if( ret > (int)sizeof(PacketResponse) )
        {
            /* Check whether this is a PacketResponse of type
             * TYPE_RESPONSE or of TYPE_LAST_RESPONSE.
             * If the type is TYPE_LAST_RESPONSE, you can leave
             * the loop after processing that last PacketResponse.
             */
            PacketResponse* pr = (PacketResponse*)buffer;
            if( ntohs(pr->type) == TYPE_RESPONSE )
            {
                printf( "%d: D2 received a RESPONSE\n", getpid() );

                /* payload should point to the first NetNode in the
                 * packet (jump over the PacketResponse header).
                 */
                char* payload = &buffer[sizeof(PacketResponse)];
                ret    -= sizeof(PacketResponse);

                /* Extract the NetNodes from the packet and store them
                 * in the LocalTreeStore. Expect up to 5 NetNodes in
                 * one packet.
                 */
                node_index = d2_add_to_local_tree( store, node_index, payload, ret );
            }
            else if( ntohs(pr->type) == TYPE_LAST_RESPONSE )
            {
                printf( "%d: D2 received a LAST RESPONSE\n", getpid() );
                repeat = 0; /* last response packet, leave loop after this one */

                /* see above */
                char* payload = &buffer[sizeof(PacketResponse)];
                ret    -= sizeof(PacketResponse);

                /* see above */
                node_index = d2_add_to_local_tree( store, node_index, payload, ret );
            }
            else
            {
                printf( "%d: D2 received neither RESPONSE nor LAST_RESPONSE\n", getpid() );
                repeat = 0;
            }
        }
    }

    /* Print the LocalTreeStore to stdout as instructed in the header file.
     */
    d2_print_tree( store );

    /* Free all resources allocated for the LocalTreeStore.
     */
    d2_free_local_tree( store );

    printf( "%d: Ending the lookup client\n", getpid() );

    /* Free all resources allocated for the association with the server.
     */
    client = d2_client_delete(client);

    return 0;
}

