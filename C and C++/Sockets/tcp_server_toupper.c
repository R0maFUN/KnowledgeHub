#include "common_incsmacs.h"
#include <ctype.h>

int main()
{

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
    hints.ai_socktype = SOCK_STREAM;
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

    printf("Listening...\n");
    if (listen(listen_socket, 10) < 0) {
        fprintf(stderr, "listen() call failed. (%d)\n", GETSOCKETERRNO());
        return 4;
    }

    fd_set master;
    FD_ZERO(&master);
    FD_SET(listen_socket, &master);
    SOCKET max_socket = listen_socket;

    printf("Waiting for connections...\n");
    while(1) {
        fd_set reads;
        reads = master;
        // time argument is 0, so select doesn't return until a socket in the master set is ready to be read from
        if (select(max_socket + 1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "select() call failed. (%d)\n", GETSOCKETERRNO());
            return 5;
        }

        SOCKET i;
        for (i = 1; i <= max_socket; ++i) {
            if (FD_ISSET(i, &reads)) {
                if (i == listen_socket) {
                    struct sockaddr_storage client_address;
                    socklen_t client_address_len = sizeof(client_address);
                    SOCKET client_socket = accept(listen_socket, (struct sockaddr*) &client_address, &client_address_len);
                    if (!ISVALIDSOCKET(client_socket)) {
                        fprintf(stderr, "accept() call failed. (%d)\n", GETSOCKETERRNO());
                        return 6;
                    }

                    FD_SET(client_socket, &master);
                    if (client_socket > max_socket) {
                        max_socket = client_socket;
                    }

                    char address_buffer[100];
                    getnameinfo((struct sockaddr*)&client_address, client_address_len,
                                address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
                    printf("New connection from %s\n", address_buffer);
                } else {
                    char read[1024];
                    int bytes_received = recv(i, read, 1024, 0);
                    if (bytes_received < 1) {
                        FD_CLR(i, &master);
                        CLOSESOCKET(i);
                        continue;
                    }

                    int j;
                    for (j = 0; j < bytes_received; ++j) {
                        read[j] = toupper(read[j]);
                    }
                    send(i, read, bytes_received, 0);
                }
            } // if FD_ISSET
        } // for i to max_socket
    } // while(1)

    printf("Closing listening socket...\n");
    CLOSESOCKET(listen_socket);

#if defined(_WIN32)
    WSACleanup();
#endif

    printf("Finished.\n");

    return 0;
}