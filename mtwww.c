#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8000

#define MAX 1024

int read_html(char *buf, char *path){
    FILE *f = fopen((char*) path, "r");
    if (f == NULL) return -1;
    
    char line[255];
    
    int flag = 0;
    while (fgets(line, sizeof(char) * MAX, f)) {
        if (flag) strcat(buf, line);
        else {
            // Overwrites garbage first iteration.
            strcpy(buf, line);
            flag = 1;
        }
    }

    printf("%s", buf);
    fclose(f);
    return 0;
}

int main (int argc, char const *argv[]) {
    if (argc < 2) {
        printf("Usage: www-path port\n");
        // exit(EXIT_FAILURE);
    }

    int server_fd;
    // Struct used when binding the socket to the address and port number specified
    struct sockaddr_in6 addr;

    // Try to create socket (AF_INET6 --> IPV6, SOCK_STREAM --> TCP)
    if ((server_fd = socket(AF_INET6, SOCK_STREAM, 0)) == 0)
    {
        printf("Could not create socket.\n");
        exit(EXIT_FAILURE);
    }

    int value = 1;

    // Sets the file descriptor for the socket.
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
        &value, sizeof(value)))
    {
        printf("Could not set socket file descriptor.\n");
        exit(EXIT_FAILURE); 
    }
    
    // Enables IPv4 and IPv6.
    addr.sin6_family = AF_INET6;
    // Any internet address.
    addr.sin6_addr = in6addr_any;
    // Sets port for socket.
    addr.sin6_port = htons( PORT );
    

    // Bind the socket to the port (8000)
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("Could not bind socket.\n");
        exit(EXIT_FAILURE);
    }
    
    // Set socket server_fd in passive mode to listen for connections.
    #define MAX_IN_QUEUE 1
    if (listen(server_fd, MAX_IN_QUEUE) < 0)
    {
        printf("Could not listen on socket.\n");
        exit(EXIT_FAILURE);
    }

    int socket_fd;
    int addr_size = sizeof(addr);

    while(1) {
        // Waits for a connection on the socket file descriptor.
        if ((socket_fd = accept(server_fd, (struct sockaddr *)&addr,
            (socklen_t*)&addr_size)) < 0)
        {
            printf("Could not connect to socket.\n");
            exit(EXIT_FAILURE);
        }

        char buffer[1024] = {0};
        if (read(socket_fd, buffer, 1024) < 0) {
            printf("Could not read from socket.\n");
        }
        
        char *reply = 
                    "HTTP/0.9 200 OK\n"
                    "Content-Type: text/html\n"
                    "Connection: close\n"
                    "\n"
                    "Hello World!";
    
        if (send(socket_fd, reply, strlen(reply), 0) < 0) {
            printf("Could not send.\n");
        }
        
        close((int) socket_fd);
    }

    char html_string[MAX];
    if (read_html(html_string, "./doc/index.html") < 0) printf("Nope\n");

    return 0;
}