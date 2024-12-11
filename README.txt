Author: Elliott Bolan
CS 3377.006
This assignment consists of three programs, the server (echo_s.c), the client (echo_c.c), and the log server (log_s.c). They support both UDP and TCP.
I made the port for my programs 7654.
To make the files:

gcc echo_s.c -o echo_s
gcc echo_c.c -o echo_c
gcc log_s.c -o log_s

ALL PROGRAMS NEED TO BE RAN ON A SEPERATE WINDOW/MACHINE ON THE UTD CS UNIX SERVER. Here are the commands to run the files:

./echo_s 7654
./echo_c 127.0.0.1 7654
./log_s

When ran:
The server listens for a connection, and echos back any messages sent

The client connects to the server, and is able to send messages. After sending a message it receives the same message (the echo) from the server. 
	It is TCP by default. To connect with UDP run it with the -u flag. To exit from TCP use CTRL+D.

The log server makes/writes to a file called echo.log all of the messages, and the client that sent them. 

