# TDT4186 P2 Multithreading and Communication

To run the web server:
1. Use a Linux distro/Unix based system
2. Compile using gcc or `./build.sh`
3. Run using `./mtwww www-path port #threads #bufferslots`, such as `./mtwww ./webroot 8000 5 10`

## a. Single-threaded web server
The setup of the socket happens in `mtwww.c` in the `main` function calling `setup_socket`. The socket is initialized to accept IPv4 and IPv6, returning a server file descriptor which is used to bind the socket to the user specified port. The server file descriptor is then set in a listening state. In the main thread, `accept` waits for connections on the port in a `while` loop, serving each request using `process_request` (is multithreaded now).

## b. Counting semaphores
Semaphores are implemented in `sem.c`.
- `sem_init` allocates memory for the semaphore and initializes its value to the specified value, and initializes its lock and condition variable.
- `sem_del` frees the allocated memory.
- `P` tries to decrement the semaphore atomically. It checks if the semaphore value is 0 or below, and waits if that is the condition. When the semaphore is >0, `P` decrements the semaphore.
- `V` increments the semaphore and signals this to the waiting threads atomically.

## c. Ring buffer
The ring buffer is implemented in `bbuffer.c`. 

- `bb_init` allocates memory and initializes the ringbuffer to be empty along with its semaphores and mutex. 
- `bb_del` frees the allocated memory (also the allocated semaphore's memory).
- `bb_get` waits on the "empty" semaphore to signal that the buffer is not empty. When the buffer has contents, `bb_get` retrieves the head-element atomically and signals to threads waiting on the "full" semaphore that the buffer is not full.
- `bb_add` waits on the "full" semaphore to signal that the buffer is not full. When the buffer is not full, `bb_add` adds the specified element to the tail of the buffer atomically and signals to threads waiting on the "empty" semaphore that the buffer is not empty.

## d. POSIX threads
Multiple workers are implemented in `mtwww.c:165`. We initialize the given amount of workers threads in a `thread_pool` array, then tell each thread to execute the `process_request` function. This function continously tries to retrieve a file descriptor from the bounded buffer `req_buffer`, using the file descriptor to process an incoming HTTP request.

The main thread, on the other hand, continously tries to add incoming HTTP requests to `req_buffer` by adding the file descriptors for the connections to the buffer.

This way the main thread acts as a *producer* accepting new requests and adding them to the buffer if it is not full, while each thread running `process_request` acts as *consumers*, processing requests from the buffer as long as it is not empty.

## e. Path traversal

To avoid the security problem of path traversal where users can GET files outside of our www-path directory, we sanitize the path in the GET requests `path_to_file`. We check for all `../` in the GET request path, and if there is any, we return a 404 Not Found error.

To test that our validation works, you can try to GET the file `cantAccessThis.html` in the project root directory.

This is just a simplified protection against path traversal. If we were to implement a stronger protection we would need to protect against `../` and `..\`, both encoded and double encoded, and all variations of it.

Another way to do this would be to use an if statement that checks if the GET request contains an approved path, only the "known good". If the path is not pre-approved, we could return a 404 Not Found error.

Reference: <https://owasp.org/www-community/attacks/Path_Traversal>
