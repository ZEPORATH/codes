//
// A simple Internet client application.
// It connects to a remote server,
// receives any messages from a server,
// prints them on standard output, and
// sends to a server all lines that a user prints 
// on the keyboard.
//
// The program illustrates the "select" function used to
// emulate an "asynchronous" style of work.
//
// Usage:
//          asyncli [IP_address_of_server [port_of_server]]
//      where IP_address_of_server is either IP number of server
//      or a symbolic Internet name, default is "localhost";
//      port_of_server is a port number, default is 1234.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>

// Socket
static int s0;  // Network socket

const int MAX_LINE_LENGTH = 65534;

static char readBuffer[MAX_LINE_LENGTH + 2];  // Read buffer

static char writeBuffer[MAX_LINE_LENGTH + 2]; // Write buffer
static char *writePos = writeBuffer;          // current position
static int writeBufferLen = 0;                // write buffer length

static int finished = 0;            // Finish the program

static void usage();
static void sigHandler(int sigID);  // Handler of SIGPIPE signal

int main(int argc, char *argv[]) {
    fd_set readfds;     // Set of socket descriptors for select
    struct timeval tv;  // Timeout value

    // Set signal handler for the "SIGPIPE" signal
    // (used to intercept the signal about broken connection).
    if (signal(SIGPIPE, &sigHandler) == SIG_ERR) {
        perror("Cannot install a signal handler");
        exit(1);
    }

    if (argc > 1 && *(argv[1]) == '-') {
        usage(); exit(1);
    }

    // Create a socket
    s0 = socket(AF_INET, SOCK_STREAM, 0);
    if (s0 < 0) {
        perror("Cannot create a socket"); exit(1);
    }

    // Fill in the address of server
    struct sockaddr_in peeraddr;
    int peeraddr_len;
    memset(&peeraddr, 0, sizeof(peeraddr));
    char* peerHost = "localhost";
    if (argc > 1)
        peerHost = argv[1];

    // Resolve the server address (convert from symbolic name to IP number)
    struct hostent *host = gethostbyname(peerHost);
    if (host == NULL) {
        perror("Cannot define host address"); exit(1);
    }
    peeraddr.sin_family = AF_INET;
    short peerPort = 1234;
    if (argc >= 3)
        peerPort = (short) atoi(argv[2]);
    peeraddr.sin_port = htons(peerPort);

    // Print a resolved address of server (the first IP of the host)
    printf(
        "peer addr = %d.%d.%d.%d, port %d\n",
        host->h_addr_list[0][0] & 0xff,
        host->h_addr_list[0][1] & 0xff,
        host->h_addr_list[0][2] & 0xff,
        host->h_addr_list[0][3] & 0xff,
        (int) peerPort
    );

    // Write resolved IP address of a server to the address structure
    memmove(&(peeraddr.sin_addr.s_addr), host->h_addr_list[0], 4);

    // Connect to a remote server
    int res = connect(s0, (struct sockaddr*) &peeraddr, sizeof(peeraddr));
    if (res < 0) {
        perror("Cannot connect"); exit(1);
    }
    printf("Connected. Type a message and press \"Enter\".\n");

    // Define socket as non-blocking
    // res = fcntl(s0, F_SETFL, O_NONBLOCK);
    // if (res < 0) {
    //     perror("Cannot set a socket as non-blocking");
    //     exit(1);
    // }

    while (!finished) {

        // Read all incoming data
        int received = 0;
        while (received < MAX_LINE_LENGTH) {
            FD_ZERO(&readfds);    // Erase the set of socket descriptors
            FD_SET(s0, &readfds); // Add the socket "s0" to the set

            tv.tv_sec = 0;        // Define the timeout value
            tv.tv_usec = 100000;  // 0.1 sec (in microseconds)

            //=== "select" is the key poit of the program! ============
            res = select(
                s0 + 1,   // Max. number of socket in all sets + 1
                &readfds, // Set of socket descriptors for reading
                NULL,     // Set of sockets for writing -- not used
                NULL,     // Set of sockets with exceptions -- not used
                &tv       // Timeout value
            );
            //=========================================================

            if (res < 0) {
                perror("Select error");
                finished = 1;
                break;
            } else if (res == 0) {
                break;  // No data is availible yet
            }

            // At this point, we can read an incoming data
            assert(FD_ISSET(s0, &readfds));

            res = read(
                s0,
                readBuffer + received,
                MAX_LINE_LENGTH - received
            );
            if (res > 0) {
                received += res;
            } else {
                if (res < 0 && errno != EAGAIN) {
                    perror("Read error");
                    finished = 1;
                }
                break;
            }
        } // end of reading

        if (received > 0) {
            readBuffer[received] = 0;
            printf("Received %d bytes:\n%s", received, readBuffer);
        }

        if (finished)
            break;

        if (writeBufferLen == 0) {
            printf("Input a line (Ctrl+D for end):\n");
            if (fgets(writeBuffer, MAX_LINE_LENGTH, stdin) != NULL) {
                writeBufferLen = strlen(writeBuffer);
                writePos = writeBuffer;
            } else {
                break;  // Break on Ctrl+D
            }
        }

        if (!finished && writeBufferLen > 0) {
            res = write(s0, writePos, writeBufferLen);
            if (res < 0) {
                if (errno != EAGAIN) {
                    perror("Write error");
                    break;              // Write error
                } else {
                    perror("Incompleted send");
                }
            } else if (res == 0) {
                printf("Connection closed");
                break;
            } else if (res > 0) {
                writePos += res;
                writeBufferLen -= res;
            }
        }
    } // end while

    shutdown(s0, 2);
    close(s0);
    return 0;
}

static void usage() {
    printf(
        "A simple Internet client application.\n"
        "Usage:\n"
        "         asyncli [IP_address_of_server [port_of_server]]\n"
        "     where IP_address_of_server is either IP number of server\n"
        "     or a symbolic Internet name, default is \"localhost\";\n"
        "     port_of_server is a port number, default is 1234.\n"
        "The client connects to a server which address is given in a\n"
        "command line, receives a messages from a server and sends\n"
        "to a server all lines that a user prints on a keyboard.\n"
    );
}

static void sigHandler(int sigID) {
    printf("The SIGPIPE signal (connection is broken).\n");
    finished = 1;
}