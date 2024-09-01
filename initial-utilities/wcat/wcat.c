#include <stdio.h>   // For standard input/output functions
#include <stdlib.h>  // For malloc(), realloc(), free(), and exit()
#include <sys/types.h>

int main(int argc, char *argv[]) {
    // Check if no files are provided
    if (argc < 2) {
        // No files to process, so just exit with success code
        return 0;
    }

    // Loop over each file name provided as command-line arguments
    for (int i = 1; i < argc; i++) {
        FILE *file = fopen(argv[i], "r");  // Open the file in read mode
        if (file == NULL) {
            // If file can't be opened, print an error message and exit with status 1
            printf("wcat: cannot open file\n");
            return 1;
        }

        // Initialize variables for dynamic memory allocation
        char *buffer = NULL;
        size_t buffer_size = 0;
        ssize_t line_length;

        // Read each line from the file until end-of-file (EOF)
        while ((line_length = getline(&buffer, &buffer_size, file)) != -1) {
            // Print the contents of the buffer (which contains a line)
            printf("%s", buffer);
        }

        // Free the dynamically allocated memory
        free(buffer);

        // Close the file after processing it
        fclose(file);
    }

    // All files processed successfully, so exit with success code
    return 0;
}
