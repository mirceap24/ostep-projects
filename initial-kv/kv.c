#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_KEY 100 
#define MAX_VALUE 1000
#define MAX_INPUT 1100 // MAX_KEY + MAX_VALUE + COMMA
#define FILENAME "database.txt"

// structure for key-value pair 
typedef struct {
    int key; 
    char value[MAX_VALUE];
} KeyValue; 

// structure for database 
typedef struct {
    KeyValue* items; 
    int capacity;
    int size; 
} Database; 

// initialize database 
Database* init_database(int initial_capacity) {
    Database* db = malloc(sizeof(Database));
    if (!db) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    db -> items = malloc(sizeof(KeyValue) * initial_capacity);
    if (!db -> items) {
        fprintf(stderr, "Memory allocation failed\n");
        free(db);
        exit(1);
    }

    db -> capacity = initial_capacity; 
    db -> size = 0; 
    return db;
}

// function to add or update key-value pair 
void put(Database* db, int key, const char* value) {
    // check if key exists 
    for (int i = 0; i < db -> size; i ++) {
        if (db -> items[i].key == key) {
            strncpy(db -> items[i].value, value, MAX_VALUE - 1);
            db -> items[i].value[MAX_VALUE - 1] = '\0';
            return;
        }
    }

    // if key doesn't exist, add new item
    if (db -> size >= db -> capacity) {
        // resize database if needed 
        db -> capacity *= 2; 
        db -> items = realloc(db -> items, sizeof(KeyValue) * db -> capacity);
        if (!db -> items) {
            fprintf(stderr, "Memory reallocation failed\n");
            exit(1);
        }
    }

    db -> items[db -> size].key = key; 
    strncpy(db -> items[db -> size].value, value, MAX_VALUE - 1);
    db -> items[db -> size].value[MAX_VALUE - 1] = '\0';
    db -> size++;
}

// function to get a value by key 
const char* get(Database* db, int key) {
    for (int i = 0; i < db -> size; i ++) {
        if (db -> items[i].key == key) {
            return db -> items[i].value;
        }
    }
    return NULL;
}

// function to delete a key-value pair 
bool delete(Database *db, int key) {
    for (int i = 0; i < db -> size; i ++) {
        if (db -> items[i].key == key) {
            // shift all elements after the found item one posiition to the left 
            for (int j = i; j < db -> size - 1; j ++) {
                db -> items[j] = db -> items[j + 1];
            }
            db -> size--;
            return true;
        }
    }
    return false;
}

// function to clear all key-value pairs 
void clear(Database* db) {
    db -> size = 0;
}

// function to print all key-value pairs 
void print_all(Database *db) {
    for (int i = 0; i < db -> size; i ++) {
        printf("%d, %s\n", db -> items[i].key, db -> items[i].value);
    }
}

// free the database memory 
void free_database(Database* db) {
    free(db -> items);
    free(db);
}

// function to save database to file 
void save_to_file(Database *db) {
    FILE* file = fopen(FILENAME, "w");
    if (!file) {
        fprintf(stderr, "Failed to open file for writing\n");
        return;
    }

    for (int i = 0; i < db -> size; i ++) {
        fprintf(file, "%d,%s\n", db->items[i].key, db->items[i].value);
    }

    fclose(file);
}

// function to load database from file 
void load_from_file(Database* db) {
    FILE* file = fopen(FILENAME, "r");
    if (!file) {
        return;
    }

    char line[MAX_KEY + MAX_VALUE + 2]; // +2 for comma and newline 
    while (fgets(line, sizeof(line), file)) {
        int key;
        char value[MAX_VALUE];
        if (sscanf(line, "%d,%[^\n]", &key, value) == 2) {
            put(db, key, value);
        }
    }
    fclose(file);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        // no arguments provided, do nothing
        return 0;
    }

    Database* db = init_database(10); // capacity for 10 items;
    load_from_file(db); // load existing data

    for (int i = 1; i < argc; i ++) {
        char* token = argv[i];

        if (token[0] == 'p' && token[1] == ',') {
            // put command 
            int key; 
            char value[MAX_VALUE];
            if (sscanf(token + 2, "%d, %[^\n]", &key, value) == 2) {
                put(db, key, value);
                save_to_file(db);
            } else {
                fprintf(stderr, "Invalid put command\n");
            }
        } else if (token[0] == 'g' && token[1] == ',') {
            // get command
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
