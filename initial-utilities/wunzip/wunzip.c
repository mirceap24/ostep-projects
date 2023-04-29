#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("wunzip: file1 [file2 ...]\n");
        exit(1);
    }

    int count;
    char character;

    for (int i = 1; i < argc; i ++) {
        FILE *file = fopen(argv[i], "r");
        if (!file) {
            perror("wunzip");
            exit(1);
        }

        while (fread(&count, sizeof(int), 1, file) == 1 && 
               fread(&character, sizeof(char), 1, file) == 1) {
                for (int j = 0; j < count; j ++) {
                    putchar(character);
                }
        }

        fclose(file);
    }

    return 0;
}