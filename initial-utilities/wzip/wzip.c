#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("wzip: file1 [file2 ...]\n");
        exit(1);
    }

    int count = 0;
    int current_char, prev_char = EOF;

    for (int i = 1; i < argc; i++) {
        FILE *file = fopen(argv[i], "r");
        if (!file) {
            perror("wzip");
            exit(1);
        }

        while ((current_char = fgetc(file)) != EOF) {
            if (current_char != prev_char && prev_char != EOF) {
                fwrite(&count, sizeof(int), 1, stdout);
                fwrite(&prev_char, sizeof(char), 1, stdout);
                count = 0;
            }
            prev_char = current_char;
            count++;
        }

        fclose(file);
    }

    if (count > 0) {
        fwrite(&count, sizeof(int), 1, stdout);
        fwrite(&prev_char, sizeof(char), 1, stdout);
    }

    return 0;
}