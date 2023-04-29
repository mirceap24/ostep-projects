#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>


void search_file(FILE *file, const char *search_term) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, file)) != -1) {
        if (strstr(line, search_term) != NULL) {
            printf("%s", line);
        }
    }

    free(line);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("wgrep: searchterm [file ...]\n");
        return 1;
    }

    const char *search_term = argv[1];

    if (argc == 2) {
        search_file(stdin, search_term);
    } else {
        for (int i = 2; i < argc; i ++) {
            FILE *file = fopen(argv[i], "r");
            if (file == NULL) {
                printf("wgrep: cannot open file\n");
                return 1;
            }

            search_file(file, search_term);
            fclose(file);
        }
    }

    return 0;
}