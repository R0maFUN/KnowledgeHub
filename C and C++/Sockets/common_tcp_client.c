#include "common_incsmacs.h"

#if defined(_WIN32)
#include <conio.h>
#endif

int main(int argc, char* argv[])
{

#if defined(_WIN32)
	WSADATA d;
	if (WSAStartup(MAKEWORD(2, 2), &d)) {
		fprintf(stderr, "Failed to initialize.\n");
		return 1;
	}
#endif

	if (argc < 3) {
		fprintf(stderr, "Usage: %s host port\n", argv[0]);
		return 2;
	}

	printf("Configuring remote address...\n");
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	struct addrinfo* peer_address;
	if (getaddrinfo(argv[1], argv[2], &hints, &peer_address)) {
		fprintf(stderr, "getaddrinfo() call failed. (%d)\n", GETSOCKETERRNO());
		return 3;
	}

	printf("Remote address resolved as: ");
	char address_buffer[100];
	char service_buffer[100];
	getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
			address_buffer, sizeof(address_buffer),
			service_buffer, sizeof(service_buffer),
			NI_NUMERICHOST);
	printf("%s %s\n", address_buffer, service_buffer);

	printf("Creating socket...\n");
	SOCKET peer_socket;
	peer_socket = socket(peer_address->ai_family, peer_address->ai_socktype, peer_address->ai_protocol);
	if (!ISVALIDSOCKET(peer_socket)) {
		fprintf(stderr, "socket() call failed. (%d)\n", GETSOCKETERRNO());
		return 4;
	}

	printf("Connecting...\n");
	if (connect(peer_socket, peer_address->ai_addr, peer_address->ai_addrlen)) {
		fprintf(stderr, "connect() call failed. (%d)\n", GETSOCKETERRNO());
		return 5;
	}
	freeaddrinfo(peer_address);

	printf("Connected.\n");
	printf("To send data, enter text followed by enter.\n");

	while(1) {
	
		fd_set reads;
		FD_ZERO(&reads);
		FD_SET(peer_socket, &reads);
#if !defined(_WIN32)
		FD_SET(fileno(stdin), &reads);
#endif

		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000; // microseconds

		if (select(peer_socket + 1, &reads, 0, 0, &timeout) < 0) {
			fprintf(stderr, "select() call failed. (%d)\n", GETSOCKETERRNO());
			return 6;
		}

		if (FD_ISSET(peer_socket, &reads)) {
			char read_buffer[4096];
			int bytes_received = recv(peer_socket, read_buffer, sizeof(read_buffer), 0);
			if (bytes_received < 1) {
				printf("Connection closed by peer.\n");
				break;
			}
			printf("Received (%d bytes): %.*s",
					bytes_received, bytes_received, read_buffer);
		}

#if defined(_WIN32)
		if (_kbhit())
#else
		if (FD_ISSET(fileno(stdin), &reads))
#endif
		{
			char read_buffer[4096];
			if (!fgets(read_buffer, sizeof(read_buffer), stdin))
				break;
			printf("Sending: %s", read_buffer);
			int bytes_sent = send(peer_socket, read_buffer, strlen(read_buffer), 0);
			printf("Sent %d bytes.\n", bytes_sent);
			// dont check connection closed error for send, cause gonna see that on the next recv
		}
	}

	printf("Closing socket...\n");
	CLOSESOCKET(peer_socket);

#if defined(_WIN32)
	WSACleanup();
#endif

	printf("Finished.\n");

	return 0;
}
