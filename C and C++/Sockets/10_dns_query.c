#include "common_incsmacs.h"

#include <stdlib.h>

#define DNS_HEADER_SIZE_BYTES 12
#define GOOGLE_DNS_HOST "8.8.8.8"
#define GOOGLE_DNS_UDP_PORT "53"

const unsigned char* print_name(const unsigned char* msg, const unsigned char* p, const unsigned char* end)
{
    if (p + 2 > end) {
        fprintf(stderr, "End of message.\n");
        exit(1);
    }

    // 0xCO = 1100 0000 | if length's 2 highest bytes are 1 -> it and the next byte should be interpreted as a pointer
    if ((*p & 0xC0) == 0xC0) {
        // 0x3F = 0011 1111
        const int k = ((*p & 0x3F) << 8) + p[1];
        p += 2;
        printf(" (pointer %d)", k);
        // print the name that is pointed to
        print_name(msg, msg + k, end);
        return p;
    } else {
        const int len = *p++;
        if (p + len + 1 > end) {
            fprintf(stderr, "End of message.\n");
            exit(1);
        }

        printf("%.*s", len, p);
        p += len;
        // name must be ended with 0
        if (*p) {
            printf(".");
            return print_name(msg, p, end);
        } else {
            return p + 1;
        }
    }
}

void print_dns_message(const char* message, int msg_length)
{
    if (msg_length < DNS_HEADER_SIZE_BYTES) {
        fprintf(stderr, "Message is too short to be valid.\n");
        exit(1);
    }

    const unsigned char* msg = (const unsigned char*) message;

    // Raw print
    // for (int i = 0; i < msg_length; ++i) {
    //     unsigned char r = msg[i];
    //     printf("%02d:  %02X %03d '%c'\n", i, r, r, r);
    // }
    // printf("\n");

    printf("ID = %0X %0X\n", msg[0], msg[1]);

    const int qr = (msg[2] & 0x80) >> 7;
    printf("QR = %d %s\n", qr, qr ? "response" : "query");

    const int opcode = (msg[2] & 0x78) >> 3;
    printf("OPCODE = %d ", opcode);
    switch (opcode) {
        case 0: printf("standard\n"); break;
        case 1: printf("reverse\n"); break;
        case 2: printf("status\n"); break;
        default: printf("?\n"); break;
    }

    const int aa = (msg[2] & 0x04) >> 2;
    printf("AA = %d %s\n", aa, aa ? "authoritative" : "");

    const int tc = (msg[2] & 0x02) >> 1;
    printf("TC = %d %s\n", tc, tc ? "message truncated" : "");

    const int rd = (msg[2] & 0x01);
    printf("RD = %d %s\n", rd, rd ? "recursion desired" : "");

    if (qr) {
        const int rcode = msg[3] & 0x07;
        printf("RCODE = %d ", rcode);
        switch (rcode) {
            case 0: printf("success\n"); break;
            case 1: printf("format error\n"); break;
            case 2: printf("server failure\n"); break;
            case 3: printf("name error\n"); break;
            case 4: printf("not implemented\n"); break;
            case 5: printf("refused\n"); break;
            default: printf("?\n"); break;
        }
        if (rcode != 0) {
            return;
        }
    }

    const int qdcount = (msg[4] << 8) + msg[5];
    const int ancount = (msg[6] << 8) + msg[7];
    const int nscount = (msg[8] << 8) + msg[9];
    const int arcount = (msg[10] << 8) + msg[11];

    printf("QDCOUNT = %d\n", qdcount);
    printf("ANCOUNT = %d\n", ancount);
    printf("NSCOUNT = %d\n", nscount);
    printf("ARCOUNT = %d\n", arcount);

    const unsigned char* p = msg + DNS_HEADER_SIZE_BYTES;
    const unsigned char* end = msg + msg_length;

    for (int i = 0; i < qdcount; ++i) {
        if (p >= end) {
            fprintf(stderr, "End of message.\n");
            exit(1);
        }

        printf("Query %2d\n", i + 1);
        printf("\tname: ");

        p = print_name(msg, p, end);
        printf("\n");

        if (p + 4 > end) {
            fprintf(stderr, "End of message.\n");
            exit(1);
        }

        const int type = (p[0] << 8) + p[1];
        printf("\ttype: %d\n", type);
        p += 2;

        const int qclass = (p[0] << 8) + p[1];
        printf("\tclass: %d\n", qclass);
        p += 2;
    }

    for (int i = 0; i < ancount + nscount + arcount; ++i) {
        if (p >= end) {
            fprintf(stderr, "End of message.\n");
            exit(1);
        }

        printf("Answer %2d\n", i + 1);
        printf("\tname: ");

        p = print_name(msg, p, end);
        printf("\n");

        if (p + 10 > end) {
            fprintf(stderr, "End of message.\n");
            exit(1);
        }

        const int type = (p[0] << 8) + p[1];
        printf("\ttype: %d\n", type);
        p += 2;

        const int qclass = (p[0] << 8) + p[1];
        printf("\tclass: %d\n", qclass);
        p += 2;

        const unsigned int ttl = (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
        printf("\tttl: %u\n", ttl);
        p += 4;

        const int rdlen = (p[0] << 8) + p[1];
        printf("\trdlen: %d\n", rdlen);
        p += 2;

        if (p + rdlen > end) {
            fprintf(stderr, "End of message.\n");
            exit(1);
        }

        if (type == 1 && rdlen == 4) { // A record
            printf("Address %d.%d.%d.%d\n", p[0], p[1], p[2], p[3]);

        } else if (type == 15 & rdlen > 3) { // MX record
            const int preference = (p[0] << 8) + p[1];
            printf("\tpref: %d\n", preference);
            printf("MX: ");
            print_name(msg, p + 2, end);
            printf("\n");

        } else if (type == 28 && rdlen == 16) { // AAAA record
            printf("Address ");
            for (int j = 0; j < rdlen; j+=2) {
                printf("%02x%02x", p[j], p[j+1]);
                if (j + 2 < rdlen) printf(":");
            }
            printf("\n");
            
        } else if (type == 16) { // TXT record
            printf("TXT: '%.*s'\n", rdlen-1, p + 1);

        } else if (type == 5) { // CNAME record
            printf("CNAME: ");
            print_name(msg, p, end);
            printf("\n");
        }
        p += rdlen;
    }

    if (p != end) {
        printf("There is some unread data left over.\n");
    }

    printf("\n");
}

int main(int argc, char* argv[]) {

    if (argc < 3) {
        printf("Usage:\n\t%s hostname type\n", argv[0]);
        printf("Example:\n\t%s example.com aaaa\n", argv[0]);
        exit(0);
    }

    if (strlen(argv[1]) > 255) {
        fprintf(stderr, "Hostname is too long.\n");
        exit(1);
    }

    unsigned char type;
    if (strcmp(argv[2], "a") == 0) {
        type = 1;
    } else if (strcmp(argv[2], "mx") == 0) {
        type = 15;
    } else if (strcmp(argv[2], "txt") == 0) {
        type = 16;
    } else if (strcmp(argv[2], "aaaa") == 0) {
        type = 28;
    } else if (strcmp(argv[2], "any") == 0) {
        type = 255;
    } else {
        fprintf(stderr, "Unknown type '%s'. Use [a, aaaa, txt, mx, any].", argv[2]);
        exit(1);
    }

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
    if (getaddrinfo(GOOGLE_DNS_HOST, GOOGLE_DNS_UDP_PORT, &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() call failed. (%d)\n", GETSOCKETERRNO());
        return 2;
    }

    printf("Creating socket...\n");
    SOCKET peer_socket;
    peer_socket = socket(peer_address->ai_family, peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(peer_socket)) {
        fprintf(stderr, "socket() call failed. (%d)\n", GETSOCKETERRNO());
        return 3;
    }

    char query[1024] = { 0xAB, 0xCD, /* ID */
                         0x01, 0x00, /* Set recursion */
                         0x00, 0x01, /* QDCOUNT */
                         0x00, 0x00, /* ANCOUNT */
                         0x00, 0x00, /* NSCOUNT */
                         0x00, 0x00 /* ARCOUNT */
                        };

    char* p = query + 12;
    char* h = argv[1];

    while (*h) {
        char* len = p;
        ++p;

        if (h != argv[1])
            ++h;

        while (*h && *h != '.')
            *p++ = *h++;

        *len = p - len - 1;
    }

    *p++ = 0;

    *p++ = 0x00; *p++ = type; // QTYPE
    *p++ = 0x00; *p++ = 0x01; // QCLASS

    const int query_size = p - query;

    int bytes_sent = sendto(peer_socket, query, query_size,
                            0,
                            peer_address->ai_addr, peer_address->ai_addrlen);
    printf("Sent %d bytes.\n", bytes_sent);

    print_dns_message(query, query_size);

    printf("Waiting for response...\n");
    char read[1024];
    int bytes_received = recvfrom(peer_socket, read, 1024, 0, 0, 0);

    printf("Received %d bytes.\n", bytes_received);

    print_dns_message(read, bytes_received);
    printf("\n");

    freeaddrinfo(peer_address);
    CLOSESOCKET(peer_socket);

#ifdef _WIN32
        WSACleanup();
#endif

    return 0;
}