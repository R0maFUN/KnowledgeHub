#include "common_incsmacs.h"

#include <ctype.h>

#define SERVICE "8080"

int main() {

#ifdef _WIN32
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif

    printf("Configuring local address..\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* bind_address;
    getaddrinfo(0, SERVICE, &hints, &bind_address);

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

    fd_set master;
    FD_ZERO(&master);
    FD_SET(listen_socket, &master);
    SOCKET max_socket = listen_socket;

    printf("Waiting for connections...\n");

    while(1) {
        fd_set reads;
        reads = master;
        struct timeval select_timeout;
        select_timeout.tv_sec = 5;
        select_timeout.tv_usec = 0;
        int select_result_code = select(max_socket + 1, &reads, 0, 0, &select_timeout);
        if (select_result_code < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 4;
        } else if (select_result_code == 0) {
            printf("select() timed out.\n");
        } else {
            printf("select() unblocked.\n");
        }

        if (FD_ISSET(listen_socket, &master)) {
            struct sockaddr_storage client_address;
            socklen_t client_address_len = sizeof(client_address);

            char read[1024];
            int bytes_received = recvfrom(listen_socket, read, 1024, 0, (struct sockaddr*)&client_address, &client_address_len);
            if (bytes_received < 1) {
                fprintf(stderr, "connection closed. (%d)\n", GETSOCKETERRNO());
                return 4;
            }

            int j;
            for (j = 0; j < bytes_received; ++j) {
                read[j] = toupper(read[j]);
            }
            sendto(listen_socket, read, bytes_received, 0, (struct sockaddr*)&client_address, client_address_len);
        } // if FD_ISSET
    } // while(1)

    printf("Closing listening socket...\n");
    CLOSESOCKET(listen_socket);

#ifdef _WIN32
    WSACleanup();
#endif

    printf("Finished.\n");

    return 0;
}