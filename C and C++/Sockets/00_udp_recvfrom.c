#include "common_incsmacs.h"

int main() {

#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif

    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* bind_address;
    getaddrinfo(0, "8080", &hints, &bind_address);

    printf("Creating socket...\n");
    SOCKET listen_socket;
    listen_socket = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(listen_socket)) {
        fprintf(stderr, "socket() call failed. (%d)\n", GETSOCKETERRNO());
        return 2;
    }

    printf("Binding socket to local address...\n");
    if (bind(listen_socket, bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() call failed. (%d)\n", GETSOCKETERRNO());
        return 3;
    }
    freeaddrinfo(bind_address);

    struct sockaddr_storage client_address;
    socklen_t client_address_len = sizeof(client_address);
    char read[1024];
    int bytes_received = recvfrom(listen_socket, read, 1024, 0, (struct sockaddr*)&client_address, &client_address_len);

    printf("Received (%d bytes): %.*s\n", bytes_received, bytes_received, read);
    
    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo((struct sockaddr*)&client_address, client_address_len,
                address_buffer, sizeof(address_buffer),
                service_buffer, sizeof(service_buffer),
                NI_NUMERICHOST | NI_NUMERICSERV);
    printf("%s %s\n", address_buffer, service_buffer);

    printf("Closing socket...\n");
    CLOSESOCKET(listen_socket);

#if defined(_WIN32)
    WSACleanup();
#endif

    printf("Finished.\n");

    return 0;
}