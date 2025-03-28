#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib") // Visual C compiler will link to lib

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#endif

#if defined(_WIN32)
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError())

#else
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define GETSOCKETERRNO() (errno)
#define SOCKET int
#endif

#if !defined(IPV6_V6ONLY)
#define IPV6_V6ONLY 27
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>

int main()
{
	
#if defined(_WIN32)
	WSADATA d;
	if (WSAStartup(MAKEWORD(2, 2), &d)) {
		fprintf(stderr, "Failed to initialize.\n");
		return 1;
	}
#endif

	printf("Ready to use socket API.\n");

	printf("Configuring local address...\n");
	struct addrinfo hints; // what we are looking for
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6; // IPv4 (AF_INET6 could be used for IPv6)
	hints.ai_socktype = SOCK_STREAM; // Going to use TCP (SOCK_DGRAM could be used for UDP)
	hints.ai_flags = AI_PASSIVE; // tell getaddrinfo that we want it to bind to the wildcard address
				     // listen on any available network interface
				     // is needed for bind()

	struct addrinfo *bind_address;
	// 80 is HTTP port, but 0 to 1023 can be used only by privileged users
	getaddrinfo(0, "8080", &hints, &bind_address);

	printf("Creating socket...\n");
	SOCKET socket_listen;
	socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);

	if (!ISVALIDSOCKET(socket_listen)) {
		fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
		return 1;
	}

	int option = 0;
	// accept both IPv4 and IPv6
	if (setsockopt(socket_listen, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&option, sizeof(option))) {
		fprintf(stderr, "setsockopt() failed. (%d)\n", GETSOCKETERRNO());
		return 5;
	}

	printf("Binding socket to local address...\n");
	if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
		fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
		return 2;
	}
	freeaddrinfo(bind_address);

	printf("Listening...\n");
	// 10 is max connections queue capacity
	if (listen(socket_listen, 10) < 0) {
		fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
		return 3;
	}

	printf("Waiting for connection...\n");
	struct sockaddr_storage client_address;
	socklen_t client_len = sizeof(client_address);
	// accept blocks thread till the new connection established
	// client_len depends on whether the connection is using IPv4 or IPv6
	SOCKET socket_client = accept(socket_listen, (struct sockaddr*) &client_address, &client_len);
	if (!ISVALIDSOCKET(socket_client)) {
		fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
		return 4;
	}

	printf("Client is connected... ");
	char address_buffer[100];
	// 0, 0 are the buffer and length for service name
	// NI_NUMERICHOST specifies we wanna see hostname as an IP address
	getnameinfo((struct sockaddr*) &client_address, client_len, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
	printf("%s\n", address_buffer);

	printf("Reading request...\n");
	char request_buffer[1024];
	int bytes_received = recv(socket_client, request_buffer, 1024, 0); // 0 - flags
	printf("Received %d bytes.\n", bytes_received);
	// the real web server should parse request and look at which resource the web browser is requesting
	
	// printf("%.*s", bytes_received, request);
	// request is not guaranteed to have null terminator
	
	printf("Sending response...\n");
	const char* response = 
		"HTTP/1.1 200 OK\r\n"
		"Connection: close\r\n"
		"Content-Type: text/plain\r\n\r\n"
		"Local time is: ";
	int bytes_sent = send(socket_client, response, strlen(response), 0); // 0 - flags
	printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(response));

	time_t timer;
	time(&timer);
	char* time_msg = ctime(&timer);
	bytes_sent = send(socket_client, time_msg, strlen(time_msg), 0);
	printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(time_msg));

	printf("Closing connection...\n");
	CLOSESOCKET(socket_client);

	printf("Closing listening socket...\n");
	CLOSESOCKET(socket_listen);


#if defined(_WIN32)
	WSACleanup();
#endif

	printf("Finished.\n");

	return 0;
}
