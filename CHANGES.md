11.4.2024
- Corrected documentation for D1Header.flags in the comments in d1_udp.h line 23-31.
- Updated documentation for several functions in d1_udp.h to specify the values that indicate sucess and error.

13.4.2024
- Corrected unclear wording in d1_send_data's comments.
- Corrected checksum computation in all servers. When the size of a packet was odd, the last byte was XORed with the wrong checksum byte.

14.4.2024
- Added [ChecksumExplanation.md](ChecksumExplanation.md) for more information about checksums.
- Clarified in d1_udp.h that the buffer passed to d1_recv_data should only contain the payload
  when the function returns, not the D1 header. d1_recv_data is the counterpart to d1_send_data,
  where this has been clarified earlier.

