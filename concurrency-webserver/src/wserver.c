#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <bits/getopt_core.h>
#include "request.h"
#include "io_helper.h"


#define MAX_BUFFER_SIZE 1024 

// structure to hold buffer infor for producer-consumer 
typedef struct {
	int *buffer;			// array to store connection file descriptors
	int buffer_size;				// size of buffer 
	int in;					// index where the next connection will be added
	int out;				// index where the next connection will be removed
	int count;				// current number of connections in buffer
	pthread_mutex_t mutex;	// mutex to protect shared access to buffer
	pthread_cond_t full; 	// condition variable to signal when buffer is full
	pthread_cond_t empty;   // condition variable to signal when buffer is empty 
} buffer_t; 

char default_root[] = ".";
buffer_t req_buffer;		// global buffer shared between the producer and consumers (workers)


// function to initialize the buffer 
void buffer_init (buffer_t *buffer, int size) {
	buffer -> buffer = (int*)malloc(sizeof(int) * size); // allocate memory for the buffer
	buffer -> buffer_size = size; 	// buffer size
	buffer -> in = 0;
	buffer -> out = 0; 
	buffer -> count = 0;
	pthread_mutex_init(&buffer->mutex, NULL);
	pthread_cond_init(&buffer->full, NULL); 
	pthread_cond_init(&buffer->empty, NULL);
}

// function to add a connection to the buffer (producer adds an item)
void buffer_add(buffer_t *buffer, int conn_fd) {
	pthread_mutex_lock(&buffer -> mutex); 	// lock the buffer 

	// wait until there is space in the buffer 
	while (buffer -> count == buffer -> buffer_size) {
		pthread_cond_wait(&buffer -> full, &buffer -> mutex); // wait if the buffer is full
	}

	// add the connection to the buffer at the `in` index 
	buffer -> buffer[buffer -> in] = conn_fd; 
	buffer -> in = (buffer -> in + 1) % buffer -> buffer_size; 	// increment `in` and wrap around if needed
	buffer -> count++;

	// signal that the buffer is not empty anymore 
	pthread_cond_signal(&buffer -> empty);  // wake up a consumer if it is waiting for items in the buffer
	pthread_mutex_unlock(&buffer -> mutex);	// unlock buffer
}

// function to remove a connection from buffer (consumer removes an item)
int buffer_remove(buffer_t *buffer) {
	pthread_mutex_lock(&buffer -> mutex);

	// wait until there is data in buffer 
	while(buffer -> count == 0) {
		pthread_cond_wait(&buffer -> empty, &buffer -> mutex); // wait if buffer is empty
	} 

	// remove the connection from the buffer at the `out` index 
	int conn_fd = buffer -> buffer[buffer -> out];
	buffer -> out = (buffer -> out + 1) % buffer -> buffer_size;
	buffer -> count --; 

	pthread_cond_signal(&buffer -> full);
	pthread_mutex_unlock(&buffer -> mutex);

	return conn_fd;
}

// Worker thread function (Consumer)
void *worker_thread(void *arg) {
    while (1) {                                           // Keep the thread alive to handle connections
        int conn_fd = buffer_remove(&req_buffer);         // Get a connection from the buffer
        request_handle(conn_fd);                          // Handle the HTTP request
        close_or_die(conn_fd);                            // Close the connection
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int c;
    char *root_dir = default_root;
    int port = 10000;
    int num_threads = 1;                                  // Default number of worker threads
    int buffer_size = 1;                                  // Default buffer size

    // Parse command-line arguments
    while ((c = getopt(argc, argv, "d:p:t:b:")) != -1) {
        switch (c) {
        case 'd':
            root_dir = optarg;                            // Set the root directory
            break;
        case 'p':
            port = atoi(optarg);                          // Set the port number
            break;
        case 't':
            num_threads = atoi(optarg);                   // Set the number of worker threads
            break;
        case 'b':
            buffer_size = atoi(optarg);                   // Set the buffer size
            break;
        default:
            fprintf(stderr, "usage: wserver [-d basedir] [-p port] [-t threads] [-b buffers]\n");
            exit(1);
        }
    }

    // Initialize the request buffer
    buffer_init(&req_buffer, buffer_size);

    // Run out of this directory
    chdir_or_die(root_dir);

    // Start worker threads
    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }

    // Main thread: handle incoming connections (Producer)
    int listen_fd = open_listen_fd_or_die(port);
    while (1) {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        int conn_fd = accept_or_die(listen_fd, (sockaddr_t *)&client_addr, (socklen_t *)&client_len);
        buffer_add(&req_buffer, conn_fd);                 // Add the connection to the buffer
    }

    // Cleanup (not typically reached in a server)
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    free(threads);
    free(req_buffer.buffer);

    return 0;
}


 
