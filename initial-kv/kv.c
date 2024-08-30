#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_KEY 100 
#define FILENAME "database.txt"

// Structure for key-value pair
typedef struct {
    int key; 
    char* value; // Dynamic memory for value
} KeyValue; 

// Structure for database 
typedef struct {
    KeyValue* items; 
    int capacity;
    int size; 
} Database; 

// Initialize database 
Database* init_database(int initial_capacity) {
    Database* db = malloc(sizeof(Database));
    if (!db) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    db->items = malloc(sizeof(KeyValue) * initial_capacity);
    if (!db->items) {
        fprintf(stderr, "Memory allocation failed\n");
        free(db);
        exit(1);
    }

    db->capacity = initial_capacity; 
    db->size = 0; 
    return db;
}

// Function to add or update key-value pair 
void put(Database* db, int key, const char* value) {
    // Check if key exists 
    for (int i = 0; i < db->size; i++) {
        if (db->items[i].key == key) {
            free(db->items[i].value); // Free the old value
            db->items[i].value = strdup(value); // Duplicate the new value
            return;
        }
    }

    // If key doesn't exist, add new item
    if (db->size >= db->capacity) {
        // Resize database if needed 
        db->capacity *= 2; 
        db->items = realloc(db->items, sizeof(KeyValue) * db->capacity);
        if (!db->items) {
            fprintf(stderr, "Memory reallocation failed\n");
            exit(1);
        }
    }

    db->items[db->size].key = key; 
    db->items[db->size].value = strdup(value); // Duplicate the value
    db->size++;
}

// Function to get a value by key 
const char* get(Database* db, int key) {
    for (int i = 0; i < db->size; i++) {
        if (db->items[i].key == key) {
            return db->items[i].value;
        }
    }
    return NULL;
}

// Function to delete a key-value pair 
bool delete(Database* db, int key) {
    for (int i = 0; i < db->size; i++) {
        if (db->items[i].key == key) {
            free(db->items[i].value); // Free the value
            // Shift all elements after the found item one position to the left 
            for (int j = i; j < db->size - 1; j++) {
                db->items[j] = db->items[j + 1];
            }
            db->size--;
            return true;
        }
    }
    return false;
}

void clear(Database* db) {
    if (db == NULL) {
        return;
    }
    for (int i = 0; i < db->size; i++) {
        free(db->items[i].value);
        db->items[i].value = NULL;
        db->items[i].key = 0;
    }
    db->size = 0;
}

// Function to print all key-value pairs 
void print_all(Database* db) {
    for (int i = 0; i < db->size; i++) {
        printf("%d,%s\n", db->items[i].key, db->items[i].value);
    }
}

void free_database(Database* db) {
    for (int i = 0; i < db->size; i++) {
        free(db->items[i].value);  
    }
    free(db->items);
    free(db);
}

// Function to save the database to a file 
void save_to_file(Database* db) {
    FILE* file = fopen(FILENAME, "w");
    if (!file) {
        fprintf(stderr, "Failed to open file for writing\n");
        return;
    }

    for (int i = 0; i < db->size; i++) {
        fprintf(file, "%d,%s\n", db->items[i].key, db->items[i].value);
    }

    fclose(file);
}

// Function to load the database from a file 
void load_from_file(Database* db) {
    FILE* file = fopen(FILENAME, "r");
    if (!file) {
        return;
    }

    char* line = NULL;
    size_t len = 0;
    while (getline(&line, &len, file) != -1) {
        int key;
        char* value = malloc(len); // Allocate memory for the value
        if (sscanf(line, "%d,%[^\n]", &key, value) == 2) {
            put(db, key, value);
        }
        free(value); // Free the temporary value buffer
    }
    free(line); // Free the line buffer
    fclose(file);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        // No arguments provided, do nothing
        return 0;
    }

    Database* db = init_database(10); // Capacity for 10 items
    load_from_file(db); // Load existing data

    for (int i = 1; i < argc; i++) {
        char* token = argv[i];

        if (token[0] == 'p' && token[1] == ',') {
            // Put command 
            int key; 
            char* value = token + 2;
            if (sscanf(token + 2, "%d, %[^\n]", &key, value) == 2) {
                put(db, key, value);
                save_to_file(db);
            } else {
                fprintf(stderr, "Invalid put command\n");
            }
        } else if (token[0] == 'g' && token[1] == ',') {
            // Get command
            int key = atoi(token + 2);
            const char* value = get(db, key);
            if (value) {
                printf("%d,%s\n", key, value);
            } else {
                printf("%d not found\n", key);
            }
        } else if (token[0] == 'd' && token[1] == ',') {
            // Delete command
            int key = atoi(token + 2);
            if (delete(db, key)) {
                save_to_file(db);
                printf("Deleted %d\n", key);
            } else {
                printf("%d not found\n", key);
            }
        } else if (strcmp(token, "c") == 0) {
            // Clear command
            clear(db);
            save_to_file(db);
            printf("Cleared all key-value pairs\n");
        } else if (strcmp(token, "a") == 0) {
            // All command
            print_all(db);
        } else {
            fprintf(stderr, "Unknown command: %s\n", token);
        }
    }

    free_database(db);
    return 0;
}
