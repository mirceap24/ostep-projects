#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <ctype.h>

#include <sys/types.h>

struct node {
    int key;
    char * value;
    struct node * next;
};

// insert or update key-value pairs in linked-list
void put(struct node ** head_ref, int key, char * value) {
    // temporary pointer to point the head of the list
    struct node * temp = * head_ref;

    // if the list is not empty and the key of the head node 
    // is equal to the key we're trying to insert, update 
    // the value of the head node with the new value and return
    if (temp != NULL && temp -> key == key) {
        temp -> value = value;
        return;
    }

    // traverse the list while the current node's key is not
    // equal to the input key 
    while (temp != NULL && temp -> key != key) {
        temp = temp -> next;
    }

    // if the key is not found in the list, create a new node
    // set its key, value and next pointer 
    // then insert it at the beginning of the list
    if (temp == NULL) {
        struct node * newNode = (struct node * ) malloc(sizeof(struct node));
        newNode -> key = key;
        newNode -> value = strdup(value);
        newNode -> next = ( * head_ref);
        ( * head_ref) = newNode;
        return;
    }

    // if the key is found update the value of the existing node
    temp -> value = value;
}

// the get function takes a pointer to the head of the linked list and an integer key as its parameters
void get(struct node * head_ref, int key) {
    if (head_ref == NULL) {
        printf("%d not found", key);
        return;
    }

    struct node * ptr = head_ref;
    // traverse the linked list until a node with a given key is found
    while (ptr -> key != key) {
        if (ptr -> next == NULL) {
            printf("%d not found\n", key);
            return;
        }
        // move pointer to the next node in the list
        ptr = ptr -> next;
    }
    // if the node with given key is found print 
    // the key and the corresponding value
    printf("%d,%s\n", ptr -> key, ptr -> value);
}

// search for a node with the specified key 
// if node is found remove it from list
void delete(struct node ** head_ref, int key) {
    struct node * current = * head_ref;
    struct node * prev = NULL;

    if (current != NULL && current -> key == key) {
        * head_ref = current -> next;
        free(current);
        return;
    }

    while (current != NULL && current -> key != key) {
        prev = current;
        current = current -> next;
    }

    if (current == NULL) {
        return;
    }

    prev -> next = current -> next;
    free(current);
}

void printAll(struct node * head_ref) {
    if (head_ref == NULL)
        return;

    struct node * current = head_ref;
    while (current != NULL) {
        printf("%d,%s\n", current -> key, current -> value);
        if (current -> next == NULL)
            return;

        current = current -> next;
    }
}

// read the contents of database.txt file 
// create a linked list using the key-value pairs from the file
int main(int argc, char * argv[]) {
    struct node * head = NULL;
    FILE * fp;
    fp = fopen("database.txt", "a+");

    // declare variables to store the line read from the file, line buffer size, and line size

    char * line = NULL;
    size_t line_buf_size = 0;
    ssize_t line_size;

    // read the file line by line until the end of the file is reached 
    while ((line_size = getline( & line, & line_buf_size, fp)) != -1) {
        // replace the newline character at the end of the line with a null character
        line[line_size - 1] = '\0';

        // split the line using the comma delimiter to get the key 
        char * key = strsep( & line, ",");
        // convert the key from a string to integer 
        int realKey = atoi(key);

        // split the line again to get the value 
        char * value = strsep( & line, ",");

        put( & head, realKey, value);

        // free the memory allocated for the line and set the line pointer to null
        // getline automatically allocates a buffer for the next line by setting line to null
        // we don't need to worry about manually managing the buffer size
        free(line);
        line = NULL;

    }
    free(line);
    line = NULL;
    fclose(fp);
    fp = NULL;

    if (argc < 2) {
        exit(0);
    }

    // iterate through each argument passed to the program
    for (int i = 1; i < argc; i++) {
        size_t len = strlen(argv[i]);
        char * command = malloc(len * sizeof(char));

        // make a copy of the current argument in the command string
        strcpy(command, argv[i]);

        // loop through the commmand string 
        // splitting it by ','
        char * argument;
        while ((argument = strsep( & command, ",")) != NULL) {
            // if the command is 'g', get the value associated with the key
            if (strcmp(argument, "g") == 0) {
                char * key, * test;
                if ((key = strsep( & command, ",")) == NULL) {
                    printf("bad command\n");
                    return 0;
                }
                if ((test = strsep( & command, ",")) != NULL) {
                    printf("bad command\n");
                    break;
                }
                int realKey = atoi(key);
                get(head, realKey); // call the 'get' function
            }
            // if the command is 'p', put a new key-value pair in the linked list
            else if (strcmp(argument, "p") == 0) {
                char * key, * value;
                if ((key = strsep(&command, ",")) == NULL) {
                    printf("bad command\n");
                    exit(0);
                }
                int realKey = atoi(key);
                if (realKey == 0) {
                    printf("bad command\n");
                    exit(0);
                }
                if ((value = strsep( & command, ",")) == NULL) {
                    printf("bad command\n");
                    exit(0);
                }
                put( & head, realKey, value);
            }
            // If the command is 'd', delete a key-value pair using the key
            else if (strcmp(argument, "d") == 0) {
                char * key;
                if ((key = strsep( & command, ",")) == NULL) {
                    printf("bad command\n");
                    exit(0);
                }
                int realKey = atoi(key);
                delete( & head, realKey); // Call the 'delete' function to remove the key-value pair
            }
            // If the command is 'c', clear the entire linked list
            else if (strcmp(argument, "c") == 0) {
                head = NULL;
            }
            // If the command is 'a', print all key-value pairs
            else if (strcmp(argument, "a") == 0) {
                printAll(head); // Call the 'printAll' function to display all key-value pairs
            } else {
                printf("bad command\n");
            }
        }
    }

    // After processing all arguments, overwrite the database file with the updated linked list
    fp = fopen("database.txt", "w");
    while (head != NULL) {
        fprintf(fp, "%d,%s\n", head -> key, head -> value);
        if (head -> next == NULL) {
            break;
        }
        head = head -> next;
    }
    fclose(fp);
    fp = NULL;

}