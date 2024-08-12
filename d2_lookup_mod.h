/* ======================================================================
 * YOU CAN MODIFY THIS FILE.
 * ====================================================================== */

#ifndef D2_LOOKUP_MOD_H
#define D2_LOOKUP_MOD_H

#include "d1_udp.h"

struct D2Client
{
    D1Peer* peer;
};

typedef struct D2Client D2Client;

struct LocalTreeStore
{
    int number_of_nodes;
    int currentNodes;
    struct NetNode** nodes;
};

typedef struct LocalTreeStore LocalTreeStore;

#endif /* D2_LOOKUP_MOD_H */