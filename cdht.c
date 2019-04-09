#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdarg.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

#define CONN_QUEUE_SIZE 20
#define MSG_SIZE 1024


int name;
int succ1;
int succ2;
int UDPsock;
int TCPsock;

// Written by
void waitFor (unsigned int secs) {
    unsigned int retTime = time(0) + secs;   // Get finishing time.
    while (time(0) < retTime);               // Loop until it arrives.
}


char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

void UDP(char *msg, int port) 
{
	struct sockaddr_in to;
	to.sin_family = AF_INET;
	to.sin_port = htons(port);
	to.sin_addr.s_addr = INADDR_ANY;
	sendto(UDPsock, msg, sizeof(msg), 0, (struct sockaddr *) &to, sizeof(struct sockaddr));
	return;
}


void *TCP(void *dest_port) 
{
	// Create new socket address
	struct sockaddr_in from;
	from.sin_family = AF_INET;
	from.sin_port = htons(name + 51000);
	from.sin_addr.s_addr = INADDR_ANY;
	
	// Set up as TCP socket
	int sockTCP = socket(AF_INET, SOCK_STREAM, 0);
	if (sockTCP == -1) {
		fprintf(stderr, "TCP Socket Creation Failure: Please Try Again\n");
		exit(1);
	}

	// Bind source socket to 51000 + i
	if (bind(sockTCP, (struct sockaddr *) &from, sizeof(struct sockaddr)) == -1) {
		fprintf(stderr, "TCP Socket Bind Failure: Please Try Again\n");
		exit(1);
	}

	// Create destination socket address struct
	int port = * (int *)dest_port;
	struct sockaddr_in to;
	to.sin_family = AF_INET;
	to.sin_port = htons(port);
	to.sin_addr.s_addr = INADDR_ANY;

	// Create TCP connection with destination
	if (connect(sockTCP, (struct sockaddr *) &to, sizeof(struct sockaddr)) == -1) {
		fprintf(stderr, "TCP Socket Connect Failure: Please Try Again\n");
		exit(1);
	}

	printf("File Sending.");
	waitFor(1);
	printf(".");
	waitFor(1);
	printf(".");
	waitFor(1);
	printf(".");
	waitFor(1);
	printf(".");
	waitFor(1);
	printf("\nFile Sent!\nClosing Connection\n");
	waitFor(1);
	close(sockTCP);

	return;
}


void *UDPlistener()
{
	struct sockaddr_in from;
	bzero(&from, sizeof(from));
	char *msg;
	int *fromlen;
	while(1) {

		recvfrom(UDPsock, msg, sizeof(msg), 0, (struct sockaddr *) &from, fromlen);
		char *token = strtok(msg, " ");
		int i = 0;
		char** tokens;
		while (token != NULL)
		{
			tokens[i++] = token;
			token = strtok(msg, " ");
		}
		int sender = ntohs(from.sin_port) - 50000;

		printf("Inside UDPlistener - tokens[0]: %s\n", tokens[0]);

		// Ping Request
		if (strcmp(tokens[0], "ping") == 0)
		{
			printf("A ping request message was received from Peer %d\n", sender);
			int sender_port = sender + 50000;
			UDP("pong", sender_port);
		}

		// Ping Response
		else if (strcmp(tokens[0], "pong") == 0)
		{
			printf("A ping response message was received from Peer %d\n", sender);
		}

		// File Request
		else if (strcmp(tokens[0], "request") == 0)
		{
			// File is here
			if (abs(atoi(tokens[2]) - name) < abs(atoi(tokens[2]) - succ1)) 
			{
				printf("File %s is here.\nA response message, destined for peer %s, has been sent.\n"
							, tokens[1], tokens[3]);
				int *dest_port;
				*dest_port = atoi(tokens[3]) + 51000;
				pthread_t TCPthread;
				pthread_create(&TCPthread, NULL, TCP, dest_port);
			}
			// File is not here
			else
			{
				printf("Passing on File Request to %d\n", succ1);
				UDP(msg, succ1 + 50000);
			}
		}

		// Leave Notification
		else if (strcmp(tokens[0], "leave") == 0)
		{
		
		}
	}
}



void *TCPlistener(void *file)
{
	while(1)
	{
		// Set up listening socket on 51000 + i
		struct sockaddr_in TCPsocket;
		TCPsocket.sin_family = AF_INET;
		TCPsocket.sin_port = htons(name + 51000);
		TCPsocket.sin_addr.s_addr = INADDR_ANY;
		memset(&(TCPsocket.sin_zero), '\0', 8); // zero the struct
		TCPsock = socket(AF_INET, SOCK_STREAM, 0);
		if (TCPsock == -1) {
			fprintf(stderr, "\nSocket Creation Failure: Please Try Again\n");
			exit(1);
		}
		if (bind(TCPsock, (struct sockaddr *) &TCPsocket, sizeof(struct sockaddr)) == -1) {
			fprintf(stderr, "\nSocket Bind Failure: You can only request one file at a time\n");
			exit(1);
		}
		if (listen(TCPsock, CONN_QUEUE_SIZE) == -1) {
			fprintf(stderr, "\nSocket Listen Failure: Please Try Again\n");
			exit(1);
		}
		struct sockaddr_in new_socket_address;
		int address_length = sizeof(struct sockaddr);
		int new_sock = accept(TCPsock, (struct sockaddr *) &new_socket_address, &address_length);
		if (new_sock == -1) {
			fprintf(stderr, "\nConnection Accept Failure: Please Try Again\n");
			exit(1);
		}
		printf("Requesting File %d\n", atoi(file));
		waitFor(4);
		close(new_sock);
	}
}

void *Initialisation()
{
	int succ1_port = succ1 + 50000;
	int succ2_port = succ2 + 50000;
	printf("sta\n");
	UDP("ping", succ1_port);
	printf("mid\n");
	UDP("ping", succ2_port);
	printf("end\n");
	return;
}


	

int main (int argc, char *argv[]) {

	if (argc != 4) {
		printf("Usage: %s [name] [peer1] [peer2]\n", argv[0]);
		return -1;
	}

	name = atoi(argv[1]);
	succ1 = atoi(argv[2]);
	succ2 = atoi(argv[3]);

	int port = name + 50000;

	// Set up socket for this peer
	struct sockaddr_in socket_address;
	socket_address.sin_family = AF_INET;
	socket_address.sin_port = htons(port);
	socket_address.sin_addr.s_addr = INADDR_ANY;
	memset(&(socket_address.sin_zero), '\0', 8); // zero the struct
	UDPsock = socket(AF_INET, SOCK_DGRAM, 0);
	if (UDPsock == -1) {
		fprintf(stderr, "Socket Creation Failure: Please Try Again\n");
		exit(1);
	}
	if (bind(UDPsock, (struct sockaddr *) &socket_address, sizeof(struct sockaddr)) == -1) {
		fprintf(stderr, "Socket Bind Failure: Please Try Again\n");
		exit(1);
	}

	// Set Up Thread For Receiving Requests
	pthread_t UDPThread;
	pthread_create(&UDPThread, NULL, UDPlistener, NULL);
	printf("returned from udp listen\n");
	waitFor(1);
	// Initial Pings to Successors
	pthread_t InitThread;
	pthread_create(&InitThread, NULL, Initialisation, NULL);
	printf("returned from initialisation\n");

	// Read From stdin During Run-Time 
	// Control of Peer from console
	char line[1024];
	size_t size = 1024;
	int succ1_port = succ1 + 50000;
	while(1) 
	{
		printf(">> ");
		// Read line-by-line
		if (fgets(line, size, stdin) == NULL) {
			fprintf(stderr, "Unable to Read From Stdin: Please Restart Peer\n");
			break;
		}

		// Split up command from stdin and decode
		char *token = strtok(line, " ");
		int i = 0;
		char** tokens;
		while (token != NULL)
		{
			tokens[i++] = token;
			token = strtok(line, " ");
		}

		// Quit
		if (strcmp(tokens[0], "quit") == 0 || strcmp(tokens[0], "quit\n") == 0)
		{
			printf("Peer %d is quitting the CDHT\n", name);
			UDP("leave", succ1_port);
		}

		// File Request
		else if (strcmp(tokens[0], "request") == 0 && tokens[1] != NULL)
		{
			// Sanity Check: File number is valid
			if (!(atoi(tokens[1]) < 10000 && atoi(tokens[1]) > 0))
				printf("Usage Error: File Must Be Integer Between 0 and 9999\n");
			else
			{
				// Open up TCP listening thread to accept TCP file transfer
				pthread_t TCPthread_listen;
				pthread_create(&TCPthread_listen, NULL, TCPlistener, &tokens[1]);

				// msg = "request XXXX hash sender"
				char msg[MSG_SIZE];
				int hash = atoi(tokens[1])%256;
				char buffer[5];
				sprintf(buffer, "%d", hash);
				strcat(strcat(strcat(msg, " "), buffer), " ");
				int TCPport = name + 51000;
				sprintf(buffer, "%d", TCPport);
				strcat(msg, buffer);

				// Send file request across UDP Socket
				UDP(msg, succ1_port);
			}
		}

		// Other
		else
			printf("Usage Error: Not a valid command\n");

	}
	return 0;
}















