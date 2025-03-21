#include "common_incsmacs.h"

#ifndef AI_ALL
#define AI_ALL 0x0100
#endif

#include <stdlib.h>

int main(int argc, char* argv[]) {

    if (argc < 2) {
        printf("Usage:\n\t%s hostname\n", argv[0]);
        printf("Example:\n\t%s example.com\n", argv[0]);
        exit(0);
    }

#ifdef _WIN32
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 2;
    }
#endif

    printf("Resolving hostname %s\n", argv[1]);
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_ALL;
    struct addrinfo* peer_address;
    if (getaddrinfo(argv[1], 0, &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() call failed. (%d)\n", GETSOCKETERRNO());
        return 3;
    }

    printf("Remote address:\n");
    struct addrinfo* address = peer_address;
    do {
        char address_buffer[100];
        getnameinfo(address->ai_addr, address->ai_addrlen,
                    address_buffer, sizeof(address_buffer),
                    0, 0, NI_NUMERICHOST);
        printf("\t%s\n", address_buffer);
    } while (address = address->ai_next);

    freeaddrinfo(peer_address);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}