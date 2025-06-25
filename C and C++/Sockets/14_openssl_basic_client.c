#include "common_incsmacs_ssl.h"

int main(int argc, char *argv[])
{

    if (argc < 3) {
		fprintf(stderr, "Usage: %s host port\n", argv[0]);
		return 2;
	}

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    // SSL_CTX_load_verify_locations() // trusted third-parties list

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        fprintf(stderr, "SSL_CTX_new() call failed().\n");
        return 1;
    }

    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *peer_address;
    if (getaddrinfo(argv[1], argv[2], &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() call failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Creating socket...\n");
    SOCKET peer_socket;
    peer_socket = socket(peer_address->ai_family, peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(peer_socket)) {
        fprintf(stderr, "socket() call failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Connecting...\n");
    if (connect(peer_socket, peer_address->ai_addr, peer_address->ai_addrlen)) {
        fprintf(stderr, "connect() call failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(peer_address);

    SSL *ssl = SSL_new(ctx);
    if (!ssl) {
        fprintf(stderr, "SSL_new() call failed.\n");
        return 1;
    }

    if (!SSL_set_tlsext_host_name(ssl, argv[1])) {
        fprintf(stderr, "SSL_set_tlsext_host_name() call failed.\n");
        return 1;
    }

    SSL_set_fd(ssl, peer_socket);
    if (SSL_connect(ssl) == -1) {
        fprintf(stderr, "SSL_connect() call failed.\n");
        ERR_print_errors_fp(stderr);
        return 1;
    }

    printf("SSL/TLS using %s\n", SSL_get_cipher(ssl));

    // char *data = "Hello, World!";
    // int bytes_sent = SSL_write(ssl, data, strlen(data));

    // char read[4096];
    // int bytes_received = SSL_read(ssl, read, 4096);
    // printf("Received: %.*s\n", bytes_received, read);

    char buffer[2048];

    sprintf(buffer, "GET / HTTP/1.1\r\n");
    sprintf(buffer + strlen(buffer), "Host: %s:%s\r\n", argv[1], argv[2]);
    sprintf(buffer + strlen(buffer), "Connection: close\r\n");
    sprintf(buffer + strlen(buffer), "User-Agent: openssl_basic_client\r\n");
    sprintf(buffer + strlen(buffer), "\r\n");

    SSL_write(ssl, buffer, strlen(buffer));
    printf("Send Headers:\n%s", buffer);

    while (1) {
        int bytes_received = SSL_read(ssl, buffer, sizeof(buffer));
        if (bytes_received < 1) {
            printf("\nConnection closed by peer.\n");
            break;
        }

        printf("Received (%d bytes): '%.*s'\n", bytes_received, bytes_received, buffer);
    }

    X509 *cert = SSL_get_peer_certificate(ssl);
    if (!cert) {
        fprintf(stderr, "SSL_get_peer_certificate() call failed.\n");
        return 1;
    }

    char *tmp;
    if (tmp = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0)) {
        printf("subject: %s\n", tmp);
        OPENSSL_free(tmp);
    }

    if (tmp = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0)) {
        printf("issuer: %s\n", tmp);
        OPENSSL_free(tmp);
    }

    X509_free(cert);

    long vp = SSL_get_verify_result(ssl);
    if (vp == X509_V_OK) {
        printf("Certificates verified successfully.\n");
    } else {
        printf("Could not verify certificates: %ld\n", vp);
    }

    SSL_shutdown(ssl);
    CLOSESOCKET(peer_socket);
    SSL_free(ssl);

    SSL_CTX_free(ctx);

    return 0;
}