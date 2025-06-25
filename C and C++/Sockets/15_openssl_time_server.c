#include "common_incsmacs_ssl.h"

int main()
{

#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        fprintf(stderr, "SSL_CTX_new call failed.\n");
        return 1;
    }

    if (!SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM)
        || !SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM)) {
        fprintf(stderr, "SSL_CTX_use_certificate_file call failed.\n");
        ERR_print_errors_fp(stderr);
        return 1;
    }

    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(0, "8080", &hints, &bind_address);

    printf("Creating socket...\n");
    SOCKET listen_socket;
    listen_socket = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(listen_socket)) {
        fprintf(stderr, "socket call failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Binding socket to local address...\n");
    if (bind(listen_socket, bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind call failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(bind_address);

    printf("Listening...\n");
    if (listen(listen_socket, 10) < 0) {
        fprintf(stderr, "listen call failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    while (1) {
        printf("Waiting for connection...\n");
        struct sockaddr_storage client_address;
        socklen_t client_address_len = sizeof(client_address);
        SOCKET client_socket = accept(listen_socket, (struct sockaddr*) &client_address, &client_address_len);
        if (!ISVALIDSOCKET(client_socket)) {
            fprintf(stderr, "accept call failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        printf("Client is connected... ");
        char address_buffer[100];
        getnameinfo((struct sockaddr*) &client_address, client_address_len, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
        printf("%s\n", address_buffer);

        SSL *ssl = SSL_new(ctx);
        if (!ssl) {
            fprintf(stderr, "SSL_new call failed.\n");
            return 1;
        }

        SSL_set_fd(ssl, client_socket);
        if (SSL_accept(ssl) <= 0) {
            fprintf(stderr, "SSL_accept call failed.\n");
            ERR_print_errors_fp(stderr);

            SSL_shutdown(ssl);
            CLOSESOCKET(client_socket);
            SSL_free(ssl);

            continue;
        }

        printf("SSL connection is using %s\n", SSL_get_cipher(ssl));

        printf("Reading request...\n");
        char request[1024];
        int bytes_received = SSL_read(ssl, request, sizeof(request));
        printf("Received %d bytes.\n", bytes_received);

        printf("Sending response...\n");
        const char *response =
            "HTTP/1.1 200 OK\r\n"
            "Connection: close\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "Local time is: ";
        int bytes_sent = SSL_write(ssl, response, strlen(response));
        printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(response));

        time_t timer;
        time(&timer);
        char *time_msg = ctime(&timer);
        bytes_sent = SSL_write(ssl, time_msg, strlen(time_msg));
        printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(time_msg));

        printf("Closing connection...\n");
        SSL_shutdown(ssl);
        CLOSESOCKET(client_socket);
        SSL_free(ssl);
    }

    printf("Closing listening socket...\n");
    CLOSESOCKET(listen_socket);
    SSL_CTX_free(ctx);

#if defined(_WIN32)
    WSACleanup();
#endif

    printf("Finished.\n");

    return 0;
}