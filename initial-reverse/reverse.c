#include <stdio.h>    // getline, fileno, fopen, fclose, fprintf
#include <stdlib.h>   // exit, malloc
#include <string.h>   // strdup
#include <sys/stat.h> // fstat
#include <sys/types.h>

// Macro for error handling: print message and exit with failure status
#define handle_error(msg)                                                      \
  do {                                                                         \
    fprintf(stderr, msg);                                                      \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

// Linked list structure for storing lines of text
typedef struct linkedList {
  char *line;            // Pointer to the text of the line
  struct linkedList *next; // Pointer to the next node in the list
} LinkedList;

int main(int argc, char *argv[]) {
  FILE *in = NULL, *out = NULL;
  in = stdin; // Default input stream is standard input
  out = stdout; // Default output stream is standard output

  // Open input and output files based on command line arguments
  if (argc == 2 && (in = fopen(argv[1], "r")) == NULL) {
    fprintf(stderr, "reverse: cannot open file '%s'\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  if (argc == 3) {
    if ((in = fopen(argv[1], "r")) == NULL) {
      fprintf(stderr, "reverse: cannot open file '%s'\n", argv[1]);
      exit(EXIT_FAILURE);
    }
    if ((out = fopen(argv[2], "w")) == NULL) {
      fprintf(stderr, "reverse: cannot open file '%s'\n", argv[2]);
      exit(EXIT_FAILURE);
    }

    // Check if input and output files are the same
    struct stat sb1, sb2;
    if (fstat(fileno(in), &sb1) == -1 || fstat(fileno(out), &sb2) == -1)
      handle_error("reverse: fstat error\n");
    if (sb1.st_ino == sb2.st_ino)
      handle_error("reverse: input and output file must differ\n");
  } else if (argc > 3)
    handle_error("usage: reverse <input> <output>\n");

  LinkedList *head = NULL; // Initialize the head of the linked list to NULL
  char *line = NULL;
  size_t len = 0;

  // Read lines from the input file and store them in the linked list in reverse order
  while (getline(&line, &len, in) != -1) {
    LinkedList *node = malloc(sizeof(LinkedList));
    if (node == NULL) {
      free(line);
      handle_error("reverse: malloc failed\n");
    }
    if ((node->line = strdup(line)) == NULL) {
      free(line);
      handle_error("reverse: strdup failed\n");
    }
    node->next = head;
    head = node;
  }

  // Iterate through the linked list and write the lines to the output file
  while (head != NULL) {
    LinkedList *temp = head;
    fprintf(out, "%s", head->line);
    head = head->next;
    // Free memory allocated for each node
    free(temp->line);
    free(temp);
  }

  // Cleanup: free memory and close files
  free(line);
  fclose(in);
  fclose(out);
  return 0;
}