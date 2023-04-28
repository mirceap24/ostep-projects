#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#define MAX_COMMAND_LENGTH 256 

void execute_cd(char **arg);
void execute_exit(char **arg);
char *execute_path(char **arg, char *path);
void check_bash_mode(int argc, char *argv[]);
void print_prompt(int interactive);
int separate_command(char command[]);
void split_command(int p);
void check_redirection(void);

char error_message[30] = "An error has occurred";

/*
 * The shell_info struct stores the current state and settings of the shell.
 * It helps to manage the shell's variables and keeps the code organized.
 * Fields:
 * - notAllowed: Flag indicating if a command is allowed to be executed (0) or not (1).
 * - interactive: Flag indicating if the shell is running in interactive mode (1) or batch mode (0).
 * - commands: Array of pointers to strings storing the commands entered by the user.
 * - args: Array of pointers to strings storing the arguments for each command entered by the user.
 * - filename: Pointer to a string containing the name of the file to be redirected, if any.
 * - outputRedirect: Flag indicating if output redirection is required (1) or not (0).
 * - multipleFiles: Flag indicating if multiple files need to be processed (1) or not (0).
 * - current_command: Integer representing the index of the current command being executed in the 'commands' array.
 * - stream: File pointer to the input stream for the shell (stdin by default, can be changed for batch mode).
 */
typedef struct shell_info
{
    int notAllowed;
    int interactive; 
    char *commands[100];
    char *args[100];
    char *filename;
    int outputRedirect; 
    int multipleFiles; 
    int current_command;
    FILE *stream;
}shell_info;

shell_info shell;

void initialize_shell(void) {
    shell.notAllowed = 0; 
    shell.interactive = 1; 
    shell.outputRedirect = 0; 
    shell.multipleFiles = 0; 
    shell.current_command = 0; 
    shell.stream = stdin;
}

/*
 * The execute_cd function changes the current working directory to the one specified by 'arg[1]'.
 * If the change is unsuccessful, it prints an error message to stderr.
 * Parameters:
 * - arg: A NULL-terminated array of pointers to strings, where arg[0] is "cd" and arg[1] is the target directory.
 */
void execute_cd(char **arg) {
    if (chdir(arg[1]) == -1) {
        fprintf(stderr, "%s\n", error_message);
    }
}

/*
 * The execute_exit function terminates the shell process if 'arg[1]' is NULL.
 * If 'arg[1]' is not NULL, it prints an error message to stderr, indicating that the 'exit' command should not have any arguments.
 * Parameters:
 * - arg: A NULL-terminated array of pointers to strings, where arg[0] is "exit" and arg[1], if present, is an invalid argument.
 */
void execute_exit(char **arg) {
    if (arg[1] != NULL) {
        fprintf(stderr, "%s\n", error_message);
    }
    else {
        exit(0);
    }
}







