#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_THREADS 64
#define CHUNK_SIZE 1048576 // 1 MB

typedef struct {
    char *file_data;           // Pointer to the memory-mapped file data, allowing threads to access the file content directly in memory.
    size_t file_size;          // The total size of the file in bytes. Used to determine the boundaries for processing.
    size_t current_pos;        // The current position within the file that has been processed or is ready to be processed by the next thread.
    pthread_mutex_t mutex;     // A mutex to synchronize access to the shared data (e.g., current_pos) among multiple threads.
    FILE *out_file;            // Pointer to the output file where the compressed data will be written. Shared by all threads, requiring synchronized access.
} SharedWorkQueue;

typedef struct {
    SharedWorkQueue *queue;    // Pointer to the SharedWorkQueue structure, giving the thread access to shared resources like file data and output file.
    int thread_id;             // An identifier for the thread, useful for distinguishing between threads. This can be used for debugging or logging.
} ThreadArg;

void *compress_chunk(void *arg);
void write_compressed_data(FILE *out, const char *data, size_t length);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file1> [file2 ...]\n", argv[0]);
        exit(1);
    }

    int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_threads > MAX_THREADS) num_threads = MAX_THREADS;

    pthread_t threads[num_threads];
    ThreadArg thread_args[num_threads];

    for (int i = 1; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if (fd == -1) {
            perror("Error opening file");
            continue;
        }

        struct stat sb;
        if (fstat(fd, &sb) == -1) {
            perror("Error getting file size");
            close(fd);
            continue;
        }

        if (sb.st_size == 0) {
            fprintf(stderr, "Error: File %s is empty.\n", argv[i]);
            close(fd);
            continue;
        }

        if (!S_ISREG(sb.st_mode)) {
            fprintf(stderr, "Error: %s is not a regular file.\n", argv[i]);
            close(fd);
            continue;
        }

        char *file_data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (file_data == MAP_FAILED) {
            perror("Error mapping file");
            close(fd);
            continue;
        }

        SharedWorkQueue queue = {
            .file_data = file_data,
            .file_size = sb.st_size,
            .current_pos = 0,
            .out_file = stdout
        };
        pthread_mutex_init(&queue.mutex, NULL);

        for (int t = 0; t < num_threads; t++) {
            thread_args[t].queue = &queue;
            thread_args[t].thread_id = t;
            if (pthread_create(&threads[t], NULL, compress_chunk, &thread_args[t]) != 0) {
                perror("Error creating thread");
                exit(1);
            }
        }

        for (int t = 0; t < num_threads; t++) {
            pthread_join(threads[t], NULL);
        }

        pthread_mutex_destroy(&queue.mutex);
        munmap(file_data, sb.st_size);
        close(fd);
    }

    return 0;
}

void *compress_chunk(void *arg) {
    ThreadArg *thread_arg = (ThreadArg *)arg;
    SharedWorkQueue *queue = thread_arg->queue;
    char *chunk = malloc(CHUNK_SIZE);
    char *output = malloc(CHUNK_SIZE * 2); 

    while (1) {
        size_t chunk_size;
        pthread_mutex_lock(&queue->mutex);

        if (queue->current_pos >= queue->file_size) {
            pthread_mutex_unlock(&queue->mutex);
            break;
        }

        // Determine the size of the chunk to be processed by this thread.
        // If the remaining portion of the file is smaller than the standard CHUNK_SIZE,
        // then set chunk_size to the remaining file size (queue->file_size - queue->current_pos).
        // Otherwise, set chunk_size to the standard CHUNK_SIZE.
        chunk_size = (queue->current_pos + CHUNK_SIZE > queue->file_size) 
                     ? (queue->file_size - queue->current_pos) 
                     : CHUNK_SIZE;

        memcpy(chunk, queue->file_data + queue->current_pos, chunk_size);
        queue->current_pos += chunk_size;

        pthread_mutex_unlock(&queue->mutex);

        size_t output_size = 0;
        char current_char = chunk[0];
        int count = 1;

        for (size_t i = 1; i < chunk_size; i++) {
            if (chunk[i] == current_char) {
                count++;
            } else {
                output_size += sprintf(output + output_size, "%c%d", current_char, count);
                current_char = chunk[i];
                count = 1;
            }
        }
        output_size += sprintf(output + output_size, "%c%d", current_char, count);

        pthread_mutex_lock(&queue->mutex);
        write_compressed_data(queue->out_file, output, output_size);
        pthread_mutex_unlock(&queue->mutex);
    }

    free(chunk);
    free(output);
    return NULL;
}

void write_compressed_data(FILE *out, const char *data, size_t length) {
    fwrite(data, 1, length, out);
}
