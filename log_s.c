// Author: Elliott Bolan
// CS 3377.006
// This program is a log server for logging the activity in the echo server into a echo.log file
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>
#define MAX_BUFFER 1024
#define LOG_SERVER_PORT 7654
#define LOG_FILE "echo.log"

// Signal handler for child processes
void sigchld_handler(int s) {
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
	int log_socket;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_len = sizeof(client_addr);
	char buffer[MAX_BUFFER];
	FILE *logFile;
	pid_t pid;

	// Create UDP socket
	log_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (log_socket < 0) {
		perror("Socket creation failed");
		exit(1);
	}

	// Allow socket reuse
	int opt = 1;
	setsockopt(log_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	// Configure server address
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(LOG_SERVER_PORT);

	// Binding socket
	if (bind(log_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("Bind failed");
		exit(1);
	}

	// Signal handling for child processes
	struct sigaction sa;
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("Log server listening on port %d\n", LOG_SERVER_PORT);

	while (1) {
		// Recieving data
		int n = recvfrom(log_socket, buffer, MAX_BUFFER, 0, (struct sockaddr*)&client_addr, &client_len);

		if (n > 0) {
			buffer[n] = '\0';
			pid = fork();

			// Child process
			if (pid == 0) {
				// Open log file in append mode
				logFile = fopen(LOG_FILE, "a");
				if (logFile == NULL) {
					perror("Could not open log file");
					exit(1);
				}

				// Write log message
				fprintf(logFile, "%s\n", buffer);
				fclose(logFile);
				exit(0);
			}
		}
	}

	// Closing the socket
	close(log_socket);
	return 0;
}
