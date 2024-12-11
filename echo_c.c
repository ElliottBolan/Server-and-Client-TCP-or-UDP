// Author: Elliott Bolan
// CS 3377.006
// This program is a client which can connect via TCP or UDP and send messages to the server.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define MAX_BUFFER 1024

int main(int argc, char *argv[]) {
	int socket_fd;
	struct sockaddr_in server_addr;
	char buffer[MAX_BUFFER];
	int port, connection_type = SOCK_STREAM;
	socklen_t addr_len;

	// Checking the argument count
	if (argc < 3 || argc > 4) {
		fprintf(stderr, "Usage: %s (-u for UDP) <server_ip> <port>\n", argv[0]);
		fprintf(stderr, "  -u: use UDP (optional)\n");
		exit(1);
	}

	// Checking for the UDP flag
	int ip_arg = 1, port_arg = 2;
	if (strcmp(argv[1], "-u") == 0) {
		connection_type = SOCK_DGRAM;
		ip_arg = 2;
		port_arg = 3;
	}

	// Parsing IP and port
	const char *server_ip = argv[ip_arg];
	port = atoi(argv[port_arg]);

	// Create socket
	socket_fd = socket(AF_INET, connection_type, 0);
	if (socket_fd < 0) {
		perror("Socket creation failed");
		exit(1);
	}

	// Configure server address
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
    
	// Convert IP address
	if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
		perror("Invalid address");
		exit(1);
	}

	// Connection for TCP, skips for UDP
	if (connection_type == SOCK_STREAM)
		if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
			perror("Connection failed");
			exit(1);
		}
	printf("Connected to %s:%d via %s\n", server_ip, port, connection_type == SOCK_STREAM ? "TCP" : "UDP");

	// Interactive communication
	while (1) {
		printf("Enter message (or Ctrl-D to exit): ");
        
		// Read from stdin and exit on CTRL-D
		if (fgets(buffer, MAX_BUFFER, stdin) == NULL)
			break;
		buffer[strcspn(buffer, "\n")] = 0;

		// Sending message
		if (connection_type == SOCK_STREAM) {
			// Send for TCP
			if (send(socket_fd, buffer, strlen(buffer), 0) < 0) {
				perror("TCP send failed");
				break;
			}

			// Receive the echo
			memset(buffer, 0, sizeof(buffer));
			int recv_len = recv(socket_fd, buffer, MAX_BUFFER - 1, 0);
			if (recv_len < 0) {
				perror("TCP receive failed");
				break;
			}
			buffer[recv_len] = '\0';
			printf("Received echo: %s\n", buffer);
		} else {
			// Send for UDP
			addr_len = sizeof(server_addr);
			if (sendto(socket_fd, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, addr_len) < 0) {
				perror("UDP send failed");
				break;
			}

			// Receive echo
			memset(buffer, 0, sizeof(buffer));
			int recv_len = recvfrom(socket_fd, buffer, MAX_BUFFER - 1, 0, (struct sockaddr*)&server_addr, &addr_len);
			if (recv_len < 0) {
				perror("UDP receive failed");
				break;
			}
			buffer[recv_len] = '\0';
			printf("Received echo: %s\n", buffer);
		}
	}
	// Close the socket
	close(socket_fd);
	return 0;
}
