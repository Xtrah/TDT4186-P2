#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "sem.h"
#include "bbuffer.h"

#define MAX_FILE_READ_LEN 1024
#define REQUEST_SIZE 65536
#define MAX_REQS_IN_QUEUE 10

BNDBUF *req_buffer;
SEM *req_prod;
SEM *req_cons;
char webroot[128];

/*
 * Function:  read_html
 * --------------------
 *  read string from path and save to buffer
 *
 *  buf: buffer to save to
 *
 *  path: file to read from
 */
int read_html(char *buf, char *path){
    FILE *f = fopen((char*) path, "r");
    if (f == NULL) return -1;
    
    char line[255];
    
    int flag = 0;
    while (fgets(line, sizeof(char) * MAX_FILE_READ_LEN, f)) {
        if (flag) strcat(buf, line);
        else {
            // Overwrites garbage first iteration.
            strcpy(buf, line);
            flag = 1;
        }
    }

    fclose(f);
    return 0;
}

// Sets up a socket connection.
int setup_socket(int *server_fd, struct sockaddr_in6 *addr, char* root, int port) {
    
    // Try to create socket (AF_INET6 --> IPV6, SOCK_STREAM --> TCP)
    if ((*server_fd = socket(AF_INET6, SOCK_STREAM, 0)) == 0)
    {
        printf("Could not create socket.\n");
        return -1;
    }

    int value = 1;

    // Sets the file descriptor for the socket.
    if (setsockopt(*server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
        &value, sizeof(value)))
    {
        printf("Could not set socket file descriptor.\n");
        return -1;
    }
    
    // Enables IPv4 and IPv6.
    addr->sin6_family = AF_INET6;
    // Any internet address.
    addr->sin6_addr = in6addr_any;
    // Sets port for socket.
    addr->sin6_port = htons(port);
    

    // Bind the socket to the port
    if (bind(*server_fd, (struct sockaddr *)addr, sizeof(*addr)) < 0)
    {
        printf("Could not bind socket.\n");
        return -1;
    }
    
    // Set socket server_fd in passive mode to listen for connections.
    if (listen(*server_fd, MAX_REQS_IN_QUEUE) < 0)
    {
        printf("Could not listen on socket.\n");
        return -1;
    }

    return 0;
}

void *process_request() {
    while (1) {
        // P(req_cons); // Wait for request to be processed.
        int socket_fd = bb_get(req_buffer);
        printf("Fd received\n");
            
        char request[REQUEST_SIZE];
        recv(socket_fd, request, REQUEST_SIZE - 1, 0);

        printf("%s", request);

        // Read lines in request header
        char *first_line = strtok(request, "\r\n");
        char *get_or_post = strtok(first_line, " ");
        char *path_to_file = strtok(NULL, " ");
        char *http_version = strtok(NULL, " ");

        char reply[1024];
        
        // If path is NOT NULL, read the file on path
        if (path_to_file != NULL) {
            char full_path[1024];
            strcpy(full_path, webroot);
            strcat(full_path, path_to_file);

            char html_string[MAX_FILE_READ_LEN];
            // Return 404 if not found
            if (read_html(html_string, full_path) < 0) {
                char error[1024] = "\nHTTP/0.9 404 Not Found\r\n"
                                    "Cotent-Type: text/html\r\n"
                                    "Connection: close\r\n"
                                    "\r\n";
                strcpy(reply, error);
            }
            // Return 200 OK if read was successfull
            else {
                char success[1024] = "\nHTTP/0.9 200 OK\r\n"
                                    "Content-Type: text/html\r\n"
                                    "Connection: close\r\n"
                                    "\r\n";

                strcat(success, html_string);
                strcpy(reply, success);
            }  

            // Try to send reply to socket
            if (send(socket_fd, reply, strlen(reply), 0) < 0) {
                printf("Could not send.\n");
            }
        }

        close((int) socket_fd);
        // V(req_prod); // Signal thread available.
    }
}

int main (int argc, char const *argv[]) {
    if (argc < 5) {
        printf("Usage: www-path port #threads #bufferslots\n");
        exit(EXIT_FAILURE);
    }
    
    // Parse arguments.
    strcpy(webroot, argv[1]);
    int port = atoi(argv[2]);

    int num_threads = atoi(argv[3]);

    // Init semaphores.
    // req_prod = sem_init(num_threads); // Possible to serve num_threads requests at a time.
    // req_cons = sem_init(0);
    req_buffer = bb_init(atoi(argv[4]));

    // Initializing thread pool according to specified thread count.
    pthread_t thread_pool[num_threads];

    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&thread_pool[i], NULL, process_request, NULL);
    }


    int server_fd;
    // Struct used when binding the socket to the address and port number specified
    struct sockaddr_in6 addr;

    if (setup_socket(&server_fd, &addr, webroot, port) < 0) {
        printf("Could not set up socket.\n");
        exit(EXIT_FAILURE);
    }


    int socket_fd;
    int addr_size = sizeof(addr);

    while(1) {
        // Waits for a connection on the socket file descriptor.
        // if ((socket_fd = accept(server_fd, (struct sockaddr *)&addr,
        //     (socklen_t*)&addr_size)) < 0)
        // {
        //     printf("Could not connect to socket.\n");
        //     exit(EXIT_FAILURE);
        // }

        // P(req_prod); // Wait for available thread.
        // Put file descriptor to new requrest into buffer.
        bb_add(req_buffer, accept(server_fd, (struct sockaddr *)&addr, (socklen_t*)&addr_size));
        printf("Request received\n");
        // V(req_cons); // Signal available request to be processed.
    }

    return 0;
}
