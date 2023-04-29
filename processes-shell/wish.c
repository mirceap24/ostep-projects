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
 *The shell_info struct stores the current state and settings of the shell.
 *It helps to manage the shell's variables and keeps the code organized.
 *Fields:
 *- notAllowed: Flag indicating if a command is allowed to be executed (0) or not (1).
 *- interactive: Flag indicating if the shell is running in interactive mode (1) or batch mode (0).
 *- commands: Array of pointers to strings storing the commands entered by the user.
 *- args: Array of pointers to strings storing the arguments for each command entered by the user.
 *- filename: Pointer to a string containing the name of the file to be redirected, if any.
 *- outputRedirect: Flag indicating if output redirection is required (1) or not (0).
 *- multipleFiles: Flag indicating if multiple files need to be processed (1) or not (0).
 *- current_command: Integer representing the index of the current command being executed in the 'commands' array.
 *- stream: File pointer to the input stream for the shell (stdin by default, can be changed for batch mode).
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
	FILE * stream;
}

shell_info;

shell_info shell;

void initialize_shell(void)
{
	shell.notAllowed = 0;
	shell.interactive = 1;
	shell.outputRedirect = 0;
	shell.multipleFiles = 0;
	shell.current_command = 0;
	shell.stream = stdin;
}

/*
 *The execute_cd function changes the current working directory to the one specified by 'arg[1]'.
 *If the change is unsuccessful, it prints an error message to stderr.
 *Parameters:
 *- arg: A NULL-terminated array of pointers to strings, where arg[0] is "cd" and arg[1] is the target directory.
 */
void execute_cd(char **arg)
{
	if (chdir(arg[1]) == -1)
	{
		fprintf(stderr, "%s\n", error_message);
	}
}

/*
 *The execute_exit function terminates the shell process if 'arg[1]' is NULL.
 *If 'arg[1]' is not NULL, it prints an error message to stderr, indicating that the 'exit' command should not have any arguments.
 *Parameters:
 *- arg: A NULL-terminated array of pointers to strings, where arg[0] is "exit" and arg[1], if present, is an invalid argument.
 */
void execute_exit(char **arg)
{
	if (arg[1] != NULL)
	{
		fprintf(stderr, "%s\n", error_message);
	}
	else
	{
		exit(0);
	}
}

/*
 *The execute_path function updates the shell's PATH environment variable by appending the specified directories
 *from the 'arg' array. This function is called when the user executes the "path" command in the shell.
 *Parameters:
 *- arg: A NULL-terminated array of pointers to strings, where arg[0] is "path" and arg[i] (i > 0) contains a
 *      directory to be added to the PATH environment variable.
 *- path: A pointer to a string representing the current value of the PATH environment variable.
 *
 *Return:
 *- pathCopy: A pointer to a string representing the updated PATH environment variable.
 */
char *execute_path(char **arg, char *path)
{
	int num_path = 0;
	char *pathCopy = strdup(path);	// copy of the current PATH 
	int i = 1;

	while (arg[i] != NULL)
	{
		/*
		 *Let's assume the initial value of path is /usr/bin:/bin, and the user wants to add the directory /opt/bin to the PATH
		 *strlen(pathCopy): calculate the length of the current path 
		 *strlen(arg[i]): calculate the length of the new directory to be added
		 *strlen(pathCopy) + strlen(arg[i]) + 2: new size for pathCopy
		 *extra 2 bytes are for the colon separator and the null-terminator
		 */
		pathCopy = realloc(pathCopy, strlen(pathCopy) + strlen(arg[i]) + 2);
		strcat(pathCopy, ":");	// add a colon separator
		strcat(pathCopy, arg[i]);	// append the new directory to pathCopy

		i++;
		num_path++;
	}

	shell.notAllowed = 0;
	return pathCopy;
}

/*
 *Check the mode of operation for the shell program based on command-line arguments
 *If the shell is running in non-interactive mode, open the specified file for reading
 *and set the shell's stream to the file stream
 */
void check_bash_mode(int argc, char *argv[])
{
	if (argc == 2)
	{
		shell.interactive = 0;
		if ((shell.stream = fopen(argv[1], "r")) == NULL)
		{
			fprintf(stderr, "%s\n", error_message);
			exit(1);
		}
	}
}

void print_prompt(int interactive)
{
	if (interactive)
	{
		printf("wish > ");
	}
}

/*
 *The separate_command function tokenizes the input string (cmd) based on the "&" delimiter. It stores the resulting
 *tokens in the shell.commands array and returns the number of tokens found. This function is used to separate
 *commands that should be executed in parallel (concurrently) in the shell.
 *
 *Parameters:
 *- cmd: A pointer to a string containing the user's input.
 *
 *Return:
 *- k: The number of commands separated by "&" found in the input string.
 */
int separate_command(char cmd[])
{
	char *command = strtok(cmd, "&");	// Tokenize the input string using the "&" delimiter
	int k = 0;	// Initialize the counter 

	// Loop throigh the tokens and store them in the shell.commands
	while (command != NULL)
	{
		shell.commands[k] = command;
		command = strtok(NULL, "&");	// Get the next command, if it exists
		k++;
	}

	return k;
}

/*
 *The split_command function tokenizes the input command string based on whitespace characters (" \t\n"). It stores the
 *resulting tokens (arguments) in the shell.args array, which can be used later to execute the command.
 *
 *Parameters:
 *- p: The index of the command in the shell.commands array.
 */
void split_command(int p)
{
	// Tokenize the command string using whitespace characters as delimiters
	char *arg = strtok(shell.commands[p], " \t\n");
	int i = 0;

	// Loop through the tokens (arguments) and store them in the shell.args array
	while (arg != NULL)
	{
		shell.args[i] = arg;
		arg = strtok(NULL, " \t\n");
		i++;
	}

	shell.args[i] = NULL;	// Mark the end of the arguments list with a NULL pointer
}

/*
 *The check_redirection function scans the arguments list (shell.args) for output redirection using the '>' symbol.
 *It sets shell.outputRedirect and shell.filename accordingly. If multiple output files are detected, it sets
 *shell.multipleFiles to 1.
 */
void check_redirection(void)
{
	int j;
	shell.multipleFiles = 0;
	shell.outputRedirect = 0;

	// Loop through the arguments list
	for (j = 0; shell.args[j] != NULL; j++)
	{
		// Check if the current argument is '>' and it is not the first argument
		if ((strcmp(shell.args[j], ">") == 0) && (strcmp(shell.args[0], ">") != 0))
		{
			// Check if there are more than one output files 
			if (shell.args[j + 2] != NULL)
			{
				shell.multipleFiles = 1;
			}
			else
			{
			 	// Set the outputRedirect flag and save the filename
				shell.outputRedirect = 1;
				shell.filename = shell.args[j + 1];
				shell.args[j] = NULL;	// Remove the '>' symbol from the arguments list
			}
		}
		else
		{
			// Check if the '>' symbol is present within the current argument
			// For cases like ls>output.txt
			char *pos = strchr(shell.args[j], '>');

			if (pos)
			{
			 	// Check if there are more than one output files 
				if (shell.args[j + 1] != NULL)
				{
					shell.multipleFiles = 1;
				}
				else
				{
				 		// Tokenize the current argument using the `>` symbol 
					char *arg = strtok(shell.args[j], ">");
					shell.args[j] = arg;

					shell.outputRedirect = 1;
					arg = strtok(NULL, ">");
					shell.filename = arg;
				}
			}
		}
	}
}

int main(int argc, char *argv[])
{
	char command[MAX_COMMAND_LENGTH];
	char *path = getenv("PATH");	// retrieve value of the environment variable "PATH"
	char *pathCopy = strdup(path);
	int num_cmd = 0;	// number of commands separated by the '&'
	int breakLoop = 0;

	initialize_shell();

	if (argc > 2)
	{
		fprintf(stderr, "%s\n", error_message);
		free(pathCopy);
		exit(1);
	}

	check_bash_mode(argc, argv);

	while (1)
	{
		// Get the user input 
		print_prompt(shell.interactive);

		// Read user input into the cmd array
		if (fgets(command, MAX_COMMAND_LENGTH, shell.stream) == NULL)
		{
			free(pathCopy);
			exit(0);
		}

		//  Remove the newline character from the end of the input by replacing it with a null character
		command[strcspn(command, "\n")] = '\0';
		if (strlen(command) == 0)
		{
			continue;
		}

		// call the separate_cmd function, which separates multiple
		// commands in the input string (if commands are separated by '&' symbol)
		num_cmd = separate_command(command);

		shell.current_command = 0;	// index of the current command being processed
		while (shell.current_command < num_cmd)
		{
			// Split input into array of arguments 

			split_command(shell.current_command);
			if (shell.args[0] == NULL)
			{
				shell.current_command++;
				continue;
			}

			check_redirection();

			if (shell.multipleFiles)
			{
				fprintf(stderr, "%s\n", error_message);
				shell.current_command++;
				continue;
			}

			if (strcmp(shell.args[0], "exit") == 0)
			{
				if (shell.args[1] != NULL)
				{
					fprintf(stderr, "%s\n", error_message);
				}
				else
				{
					breakLoop = 1;
					break;
				}
			}
			else if (strcmp(shell.args[0], "cd") == 0)
			{
				execute_cd(shell.args);
			}
			else if (strcmp(shell.args[0], "path") == 0)
			{
				if (shell.args[1] == NULL)
				{
					shell.notAllowed = 1;
				}
				else
				{
					pathCopy = execute_path(shell.args, path);
				}
			}
			else if (!shell.notAllowed)
			{
			 	// Fork a child process to execute the command
				pid_t pid = fork();

				if (pid == 0)
				{
				 		// Child process: execute the command
					char *fullpath;
					char *token;
					int i = 0;
					int found = 0;
					while (!found && (token = strtok(pathCopy, ":")) != NULL)
					{
						i++;
						pathCopy = NULL;
						fullpath = malloc(strlen(token) + strlen(shell.args[0]) + 2);
						sprintf(fullpath, "%s/%s", token, shell.args[0]);
						if (access(fullpath, X_OK) == 0)
						{

							if (shell.outputRedirect)
							{
								int output_fd = open(shell.filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
								if (output_fd == -1)
								{
									fprintf(stderr, "%s\n", error_message);
									free(pathCopy);
									exit(EXIT_FAILURE);
								}

								if (dup2(output_fd, STDOUT_FILENO) == -1)
								{
									fprintf(stderr, "%s\n", error_message);
									free(pathCopy);
									exit(EXIT_FAILURE);
								}
							}

							execv(fullpath, shell.args);
							perror(shell.args[0]);
							free(fullpath);
							//close(output_fd);
							free(pathCopy);
							exit(EXIT_FAILURE);
						}

						free(fullpath);
					}

					if (i == 0)
					{
						fprintf(stderr, "%s\n", error_message);
					}
					else
					{
						fprintf(stderr, "%s\n", error_message);
					}

					free(pathCopy);
					exit(EXIT_FAILURE);
				}
				else if (pid < 0)
				{
				 		// Fork error
					fprintf(stderr, "%s\n", error_message);
				}
				else
				{
				 		// Parent process:
				}
			}
			else
			{
				fprintf(stderr, "%s\n", error_message);
			}

			shell.current_command++;
		}

		if (breakLoop)
		{
			break;
		}

		while (wait(NULL) > 0);
	}

	free(pathCopy);
	return 0;
}