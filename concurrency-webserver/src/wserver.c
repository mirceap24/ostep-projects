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
#define MAXBUF (8192)

typedef struct {
	int conn_fd;			// connection file descriptor
	size_t file_size; 		// size of the file to be served 
} request_t; 

// structure to hold buffer info for producer-consumer 
typedef struct {
	request_t *buffer; 		// array to store connection file descriptors and file sizes
	int buffer_size;		// size of buffer 
	int in;					// index where the next connection will be added
	int out;				// index where the next connection will be removed
	int count;				// current number of connections in buffer
	pthread_mutex_t mutex;	// mutex to protect shared access to buffer
	pthread_cond_t full; 	// condition variable to signal when buffer is full
	pthread_cond_t empty;   // condition variable to signal when buffer is empty 
} buffer_t; 

char default_root[] = ".";
buffer_t req_buffer;		// global buffer shared between the producer and consumers (workers)
int schedalg = 0;			// scheduling algorithm (0 for FIFO, 1 for SFF)

// function to initialize the buffer 
void buffer_init (buffer_t *buffer, int size) {
	buffer -> buffer = (request_t*)malloc(sizeof(request_t) * size); // allocate memory for the buffer
	buffer -> buffer_size = size; 	// buffer size
	buffer -> in = 0;
	buffer -> out = 0; 
	buffer -> count = 0;
	pthread_mutex_init(&buffer->mutex, NULL);
	pthread_cond_init(&buffer->full, NULL); 
	pthread_cond_init(&buffer->empty, NULL);
}

// read request's filename 
void read_request_filename(int conn_fd, char *filename) {
	char buf[MAXBUF], method[MAXBUF], uri[MAXBUF], version[MAXBUF];
	readline_or_die(conn_fd, buf, MAXBUF);
	sscanf(buf, "%s %s %s", method, uri, version);
	request_parse_uri(uri, filename, NULL); 
}

// function to add a connection to the buffer (producer adds an item)
void buffer_add(buffer_t *buffer, int conn_fd) {
	pthread_mutex_lock(&buffer -> mutex);

	// wait until there is space in buffer 
	while (buffer -> count == buffer -> buffer_size) {
		pthread_cond_wait(&buffer->full, &buffer->mutex); // wait if the buffer is full
	}

	// calculate file size for SFF 
	size_t file_size = 0; 
	if (schedalg == 1) {
		char filename[MAXBUF];
		read_request_filename(conn_fd, filename);
		struct stat file_stat; 
		if (stat(filename, &file_stat) == 0) {
			file_size = file_stat.st_size;
		}
	}

	// add the connection and file size to the buffer at the `in` index 
	buffer -> buffer[buffer -> in].conn_fd = conn_fd;
	buffer -> buffer[buffer -> in].file_size = file_size; 
	buffer -> in = (buffer -> in + 1) % (buffer -> buffer_size);
	buffer -> count++;

	// signal that buffer is not empty anymore 
	pthread_cond_signal(&buffer->empty);
    pthread_mutex_unlock(&buffer->mutex); 
}

// function to remove a connection from buffer (consumer removes an item)
int buffer_remove(buffer_t *buffer) {
	pthread_mutex_lock(&buffer -> mutex);

	// wait until there is data in buffer 
	while (buffer -> count == 0) {
		pthread_cond_wait(&buffer -> empty, &buffer -> mutex); // wait if buffer empty
	}

	int selected_index = buffer -> out; // FIFO by default 

	if (schedalg == 1) { // SFF
		// find index of the request with the smallest file size
		size_t smallest_size = buffer -> buffer[buffer -> out].file_size;
		for (int i = 1; i < buffer -> count; i ++) {
			int index = (buffer -> out + i) % buffer -> buffer_size;
			if (buffer -> buffer[index].file_size < smallest_size) {
				smallest_size = buffer -> buffer[index].file_size; 
				selected_index = index;
			}
		}
	}

	// remove connection from the buffer 
	int conn_fd = buffer -> buffer[selected_index].conn_fd;

	// adjust the buffer if we are not removing the front 
	if (selected_index != buffer -> out) {
		// shift all elements to fill the gap left by the removed element 
		for (int i = selected_index; i != buffer -> in; i = (i + 1) % buffer -> buffer_size) {
			int next_index = (i + 1) % buffer -> buffer_size;
			buffer -> buffer[i] = buffer -> buffer[next_index];
		}
		buffer -> in = (buffer -> in + buffer -> buffer_size - 1) % buffer -> buffer_size;
	}

	buffer -> out = (buffer->out + 1) % buffer->buffer_size;
	buffer -> count--;

	// signal that buffer is not full anymore 
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
    while ((c = getopt(argc, argv, "d:p:t:b:s")) != -1) {
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
		case 's':
			if (strcmp(optarg, "SFF") == 0) {
				schedalg = 1;
			} else {
				schedalg = 0;
			}
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