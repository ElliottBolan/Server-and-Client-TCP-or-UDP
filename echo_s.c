// Author: Elliott Bolan
// CS 3377.006
// This program is a server. It can handle TCP and UDP connections, and it sends log information to the log server.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#define MAX_BUFFER 1024
#define LOG_SERVER_PORT 7654

// Sending log messages to log server
void send_log_message(const char* client_ip, const char* message) {
	int log_socket;
	struct sockaddr_in log_addr;
	time_t now;
	char timestamp[64];
	char log_message[MAX_BUFFER];

	// Create UDP socket
	log_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (log_socket < 0) {
		perror("Log socket creation failed");
		return;
	}

	// Configure log server address
	memset(&log_addr, 0, sizeof(log_addr));
	log_addr.sin_family = AF_INET;
	log_addr.sin_port = htons(LOG_SERVER_PORT);
	log_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// Get current timestamp
	time(&now);
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

	// Formatting the log message
	snprintf(log_message, sizeof(log_message), "%s \"%s\" was received from %s", timestamp, message, client_ip);

	// Sending the log message
	sendto(log_socket, log_message, strlen(log_message), 0, (struct sockaddr*)&log_addr, sizeof(log_addr));

	// Closing the log socket
	close(log_socket);
}

// Signal handler for child processes
void sigchld_handler(int s) {
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[]) {
	int tcp_socket, udp_socket, new_socket;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_len = sizeof(client_addr);
	char buffer[MAX_BUFFER];
	int port, n;
	pid_t pid;
	fd_set read_fds;

	// Check command line arguments
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		exit(1);
	}
	port = atoi(argv[1]);

	// Setup the TCP socket
	tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp_socket < 0) {
		perror("TCP socket creation failed");
		exit(1);
	}

	// Setup the UDP socket
	udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_socket < 0) {
		perror("UDP socket creation failed");
		exit(1);
	}

	// Allow socket reuse
	int opt = 1;
	setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	// Configure server address
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	// Bind TCP socket
	if (bind(tcp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("TCP bind failed");
		exit(1);
	}

	// Bind UDP socket
	if (bind(udp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("UDP bind failed");
		exit(1);
	}

	// Listen for TCP connections
	listen(tcp_socket, 5);

	// Setup signal handling for child processes
	struct sigaction sa;
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("Echo server listening on port %d (TCP and UDP)\n", port);

	while (1) {
	        // Reset file descriptor set
		FD_ZERO(&read_fds);
		FD_SET(tcp_socket, &read_fds);
		FD_SET(udp_socket, &read_fds);

		// Select to handle both TCP and UDP
		int max_fd = (tcp_socket > udp_socket) ? tcp_socket : udp_socket;
		if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			continue;
		}

		// Recieving and sending TCP messages
		if (FD_ISSET(tcp_socket, &read_fds)) {
			new_socket = accept(tcp_socket, (struct sockaddr*)&client_addr, &client_len);
			if (new_socket < 0) {
				perror("TCP accept failed");
				continue;
			}

			pid = fork();
			// Child Process
			if (pid == 0) {
				close(tcp_socket);
				close(udp_socket);

				char client_ip[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

				while ((n = recv(new_socket, buffer, MAX_BUFFER, 0)) > 0) {
					buffer[n] = '\0'; 
					send(new_socket, buffer, n, 0);
					send_log_message(client_ip, buffer);
				}

				close(new_socket);
				exit(0);
			}
			close(new_socket);
		}

        // Handle UDP datagrams
	if (FD_ISSET(udp_socket, &read_fds)) {
		n = recvfrom(udp_socket, buffer, MAX_BUFFER, 0, 
		(struct sockaddr*)&client_addr, &client_len);
			if (n > 0) {
				buffer[n] = '\0';  // Null terminate

				char client_ip[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

				// Echo back the message
				sendto(udp_socket, buffer, n, 0, 
				(struct sockaddr*)&client_addr, client_len);

				send_log_message(client_ip, buffer);
	 		}
		}
	}
	close(tcp_socket);
	close(udp_socket);
	return 0;
}
