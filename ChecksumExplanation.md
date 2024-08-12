Checksum logic can be confusing and seem weird, but the principle used here is the same as with real-world protocols:
 - create the array of bytes exactly in the order that will be sent over the network (after conversion)
 - set the bytes in this array that will contain the checksum to 0
 - compute the checksum
 - fill it into the array into the right places
 - send the packet

This is less weird when the checksum is in the trailer, but most modern protocols store it in the header.

The following are examples of the byte sequences that are sent over the network for the given situations.

```
ACK packet, ackno 0
    -> 01 00 01 08 00 00 00 08
        ^  ^  ^  ^  ^  ^  ^  ^
        |  |  |  |  |  |  |  |
        |  |  |  |  +--+--+--+--- size in network byte order
        |  |  |  |
        |  |  +--+--- checksum, uses the order of the packet, first byte is XOR of the odd bytes, second of the even bytes
        |  |
        +--+--- flags in network byte order

ACK packet, ackno 1
    -> 01 01 01 09 00 00 00 08

Data packet, the payload is empty, seqno 0
    -> 80 00 80 08 00 00 00 08

Data packet, the payload is empty, seqno 1
    -> 80 80 80 88 00 00 00 08

Data packet, payload 1 byte, payload[0]=1, seqno 0
    -> 80 00 81 09 00 00 00 09 01

Data packet, paylod 2 bytes, payload[0]=0, payload[1]=1, seqno 0
    -> 80 00 80 0b 00 00 00 0a 00 01

Data packet, paylod 3 bytes, payload[0]=0xff, payload[1]=0x0b, payload[2]=0x7f, seqno 0
    -> 80 00 00 00 00 00 00 0b ff 0b 7f

Data packet, paylod 3 bytes, payload[0]=0xfa, payload[1]=0x74, payload[2]=0x85, seqno 1
    -> 80 80 ff ff 00 00 00 0b fa 74 85

Data packet, payload is uint32_t containing the value 65791 (hex 100ff) in network byte order, seqno 1
    -> 80 80 80 72 00 00 00 0c 00 01 00 ff
```
