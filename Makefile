CFLAGS=-g -std=gnu11 -Wall -Wextra
LDFLAGS=-g

all: libhe.a d1_test_client d2_test_client

libhe.a: d1_udp.o d2_lookup.o
	ar rc $@ $^

d1_test_client: d1_test_client.o libhe.a
	gcc $(LDFLAGS) -o $@ $^

d2_test_client: d2_test_client.o libhe.a
	gcc $(LDFLAGS) -o $@ $^

d1_udp.o: d1_udp.c d1_udp.h d1_udp_mod.h

d2_lookup.o: d2_lookup.c d2_lookup.h d1_udp.h d1_udp_mod.h

d1_test_client.o: d1_test_client.c
d1_test_client.o: d1_udp.h d1_udp_mod.h

d2_test_client.o: d2_test_client.c
d2_test_client.o: d1_udp.h d1_udp_mod.h d2_lookup.h

%.o: %.c
	gcc $(CFLAGS) -c $^

clean:
	rm -f d1_test_client
	rm -f d2_test_client
	rm -f *.o
	rm -f libhe.a
	rm -rf *.gch


