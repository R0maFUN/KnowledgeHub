#include "common_incsmacs.h"

#define HOST "127.0.0.1"
#define SERVICE "8080"

int main() {

#ifdef _WIN32
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif

    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    struct addrinfo* peer_address;
    if (getaddrinfo(HOST, SERVICE, &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() call failed. (%d)\n", GETSOCKETERRNO());
        return 2;
    }

    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
                address_buffer, sizeof(address_buffer),
                service_buffer, sizeof(service_buffer),
                NI_NUMERICHOST | NI_NUMERICSERV);
    printf("%s %s\n", address_buffer, service_buffer);

    printf("Creating socket...\n");
    SOCKET peer_socket;
    peer_socket = socket(peer_address->ai_family, peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(peer_socket)) {
        fprintf(stderr, "socket() call failed. (%d)\n", GETSOCKETERRNO());
        return 3;
    }

    const char* message = "Hello, World!";
    printf("Sending: %s\n", message);
    int bytes_sent = sendto(peer_socket, message, strlen(message),
                            0,
                            peer_address->ai_addr, peer_address->ai_addrlen);
    printf("Sent %d bytes.\n", bytes_sent);

    freeaddrinfo(peer_address);
    CLOSESOCKET(peer_socket);

#ifdef _WIN32
    WSACleanup();
#endif

    printf("Finished.\n");

    return 0;
}