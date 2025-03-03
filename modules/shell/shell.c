/**
 * shell.c - Shell implementation for NeuroOS
 * 
 * This file implements the Shell subsystem, which provides a command-line
 * interface for the operating system.
 */

#include "shell/shell.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

// Shell state
static int shell_initialized = 0;
static int shell_running = 0;
static char shell_prompt[64] = "NeuroOS> ";
static char shell_history[SHELL_MAX_HISTORY][SHELL_MAX_COMMAND_LENGTH];
static size_t shell_history_count = 0;
static size_t shell_history_index = 0;

// Shell commands
static shell_command_t shell_commands[64];
static size_t shell_command_count = 0;

// Forward declarations of static functions
static int shell_find_command(const char* name);
static int shell_execute_builtin(const char* name, int argc, char** argv);
static int shell_execute_external(const char* name, int argc, char** argv);
static int shell_find_executable(const char* name, char* path, size_t path_size);

/**
 * Initialize the Shell subsystem
 * 
 * @return: 0 on success, -1 on failure
 */
int shell_init(void) {
    // Check if the Shell is already initialized
    if (shell_initialized) {
        return 0;
    }
    
    // Initialize the shell state
    shell_initialized = 1;
    shell_running = 0;
    shell_history_count = 0;
    shell_history_index = 0;
    
    // Register built-in commands
    shell_register_command("help", "Display help information", shell_cmd_help);
    shell_register_command("exit", "Exit the shell", shell_cmd_exit);
    shell_register_command("echo", "Display a line of text", shell_cmd_echo);
    shell_register_command("cd", "Change the current directory", shell_cmd_cd);
    shell_register_command("pwd", "Print the current working directory", shell_cmd_pwd);
    shell_register_command("ls", "List directory contents", shell_cmd_ls);
    shell_register_command("cat", "Concatenate and display files", shell_cmd_cat);
    shell_register_command("mkdir", "Create directories", shell_cmd_mkdir);
    shell_register_command("rmdir", "Remove directories", shell_cmd_rmdir);
    shell_register_command("rm", "Remove files or directories", shell_cmd_rm);
    shell_register_command("cp", "Copy files or directories", shell_cmd_cp);
    shell_register_command("mv", "Move or rename files or directories", shell_cmd_mv);
    shell_register_command("touch", "Change file timestamps", shell_cmd_touch);
    shell_register_command("grep", "Search for patterns in files", shell_cmd_grep);
    shell_register_command("find", "Search for files in a directory hierarchy", shell_cmd_find);
    shell_register_command("history", "Display command history", shell_cmd_history);
    shell_register_command("clear", "Clear the terminal screen", shell_cmd_clear);
    shell_register_command("date", "Display the current date and time", shell_cmd_date);
    shell_register_command("ps", "Report process status", shell_cmd_ps);
    shell_register_command("kill", "Send a signal to a process", shell_cmd_kill);
    shell_register_command("exec", "Execute a command", shell_cmd_exec);
    
    return 0;
}

/**
 * Shutdown the Shell subsystem
 * 
 * @return: 0 on success, -1 on failure
 */
int shell_shutdown(void) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return 0;
    }
    
    // Reset the shell state
    shell_initialized = 0;
    shell_running = 0;
    shell_command_count = 0;
    
    return 0;
}

/**
 * Register a shell command
 * 
 * @param name: Command name
 * @param description: Command description
 * @param func: Command function
 * @return: 0 on success, -1 on failure
 */
int shell_register_command(const char* name, const char* description, shell_command_func_t func) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if the command already exists
    if (shell_find_command(name) != -1) {
        return -1;
    }
    
    // Check if there's room for another command
    if (shell_command_count >= sizeof(shell_commands) / sizeof(shell_commands[0])) {
        return -1;
    }
    
    // Register the command
    shell_commands[shell_command_count].name = name;
    shell_commands[shell_command_count].description = description;
    shell_commands[shell_command_count].func = func;
    shell_command_count++;
    
    return 0;
}

/**
 * Unregister a shell command
 * 
 * @param name: Command name
 * @return: 0 on success, -1 on failure
 */
int shell_unregister_command(const char* name) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Find the command
    int index = shell_find_command(name);
    
    if (index == -1) {
        return -1;
    }
    
    // Remove the command
    for (size_t i = index; i < shell_command_count - 1; i++) {
        shell_commands[i] = shell_commands[i + 1];
    }
    
    shell_command_count--;
    
    return 0;
}

/**
 * Execute a shell command
 * 
 * @param command: Command to execute
 * @return: Command exit code
 */
int shell_execute_command(const char* command) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if the command is empty
    if (!command || !*command) {
        return 0;
    }
    
    // Add the command to the history
    shell_add_to_history(command);
    
    // Parse the command
    int argc = 0;
    char* argv[SHELL_MAX_ARGS];
    
    if (shell_parse_command(command, &argc, argv) != 0) {
        shell_printf("Error: Failed to parse command\n");
        return -1;
    }
    
    // Check if there are any arguments
    if (argc == 0) {
        return 0;
    }
    
    // Find the command
    int index = shell_find_command(argv[0]);
    
    if (index != -1) {
        // Execute the built-in command
        return shell_commands[index].func(argc, argv);
    } else {
        // Execute the external command
        return shell_execute_external(argv[0], argc, argv);
    }
}

/**
 * Execute a shell script
 * 
 * @param script_path: Path to the script
 * @return: Script exit code
 */
int shell_execute_script(const char* script_path) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if the script path is valid
    if (!script_path || !*script_path) {
        return -1;
    }
    
    // Open the script file
    FILE* file = fopen(script_path, "r");
    
    if (!file) {
        shell_printf("Error: Failed to open script file '%s'\n", script_path);
        return -1;
    }
    
    // Read and execute each line
    char line[SHELL_MAX_COMMAND_LENGTH];
    int exit_code = 0;
    
    while (fgets(line, sizeof(line), file)) {
        // Remove the trailing newline
        size_t len = strlen(line);
        
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        // Execute the command
        exit_code = shell_execute_command(line);
        
        // Check if the command failed
        if (exit_code != 0) {
            break;
        }
    }
    
    // Close the script file
    fclose(file);
    
    return exit_code;
}

/**
 * Print formatted output to the shell
 * 
 * @param format: Format string
 * @param ...: Format arguments
 * @return: Number of characters printed
 */
int shell_printf(const char* format, ...) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Format the output
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);
    
    return result;
}

/**
 * Read a line of input from the shell
 * 
 * @param buffer: Buffer to store the input
 * @param size: Buffer size
 * @return: Number of characters read
 */
int shell_gets(char* buffer, size_t size) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if the buffer is valid
    if (!buffer || size == 0) {
        return -1;
    }
    
    // Read a line of input
    if (fgets(buffer, size, stdin) == NULL) {
        return -1;
    }
    
    // Remove the trailing newline
    size_t len = strlen(buffer);
    
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    }
    
    return len;
}

/**
 * Read a character from the shell
 * 
 * @return: Character read
 */
int shell_getchar(void) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Read a character
    return getchar();
}

/**
 * Write a character to the shell
 * 
 * @param c: Character to write
 * @return: Character written
 */
int shell_putchar(int c) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Write a character
    return putchar(c);
}

/**
 * Add a command to the shell history
 * 
 * @param command: Command to add
 * @return: 0 on success, -1 on failure
 */
int shell_add_to_history(const char* command) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if the command is valid
    if (!command || !*command) {
        return -1;
    }
    
    // Add the command to the history
    strncpy(shell_history[shell_history_index], command, SHELL_MAX_COMMAND_LENGTH - 1);
    shell_history[shell_history_index][SHELL_MAX_COMMAND_LENGTH - 1] = '\0';
    
    shell_history_index = (shell_history_index + 1) % SHELL_MAX_HISTORY;
    
    if (shell_history_count < SHELL_MAX_HISTORY) {
        shell_history_count++;
    }
    
    return 0;
}

/**
 * Clear the shell history
 * 
 * @return: 0 on success, -1 on failure
 */
int shell_clear_history(void) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Clear the history
    shell_history_count = 0;
    shell_history_index = 0;
    
    return 0;
}

/**
 * Get the shell history
 * 
 * @param history: Array to store the history
 * @param count: Pointer to store the history count
 * @return: 0 on success, -1 on failure
 */
int shell_get_history(char** history, size_t* count) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if the history and count pointers are valid
    if (!history || !count) {
        return -1;
    }
    
    // Get the history
    *count = shell_history_count;
    
    for (size_t i = 0; i < shell_history_count; i++) {
        size_t index = (shell_history_index - shell_history_count + i) % SHELL_MAX_HISTORY;
        history[i] = shell_history[index];
    }
    
    return 0;
}

/**
 * Parse a shell command
 * 
 * @param command: Command to parse
 * @param argc: Pointer to store the argument count
 * @param argv: Array to store the arguments
 * @return: 0 on success, -1 on failure
 */
int shell_parse_command(const char* command, int* argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if the command, argc, and argv pointers are valid
    if (!command || !argc || !argv) {
        return -1;
    }
    
    // Parse the command
    *argc = 0;
    
    // Skip leading whitespace
    while (*command && (*command == ' ' || *command == '\t')) {
        command++;
    }
    
    // Parse the arguments
    while (*command && *argc < SHELL_MAX_ARGS) {
        // Store the argument
        argv[(*argc)++] = (char*)command;
        
        // Skip to the end of the argument
        while (*command && *command != ' ' && *command != '\t') {
            command++;
        }
        
        // Null-terminate the argument
        if (*command) {
            *(char*)command++ = '\0';
            
            // Skip whitespace
            while (*command && (*command == ' ' || *command == '\t')) {
                command++;
            }
        }
    }
    
    return 0;
}

/**
 * Find a shell command
 * 
 * @param name: Command name
 * @return: Command index on success, -1 if not found
 */
static int shell_find_command(const char* name) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if the name pointer is valid
    if (!name || !*name) {
        return -1;
    }
    
    // Find the command
    for (size_t i = 0; i < shell_command_count; i++) {
        if (strcmp(shell_commands[i].name, name) == 0) {
            return i;
        }
    }
    
    return -1;
}

/**
 * Execute a built-in shell command
 * 
 * @param name: Command name
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
static int shell_execute_builtin(const char* name, int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Find the command
    int index = shell_find_command(name);
    
    if (index == -1) {
        return -1;
    }
    
    // Execute the command
    return shell_commands[index].func(argc, argv);
}

/**
 * Execute an external shell command
 * 
 * @param name: Command name
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
/**
 * Find an executable in the PATH
 * 
 * @param name: Executable name
 * @param path: Buffer to store the executable path
 * @param path_size: Buffer size
 * @return: 0 on success, -1 on failure
 */
static int shell_find_executable(const char* name, char* path, size_t path_size) {
    // Check if the name and path pointers are valid
    if (!name || !path || path_size == 0) {
        return -1;
    }
    
    // Check if the name contains a path separator
    if (strchr(name, '/') != NULL) {
        // Use the name as-is
        strncpy(path, name, path_size - 1);
        path[path_size - 1] = '\0';
        
        // Check if the file exists and is executable
        if (access(path, X_OK) == 0) {
            return 0;
        }
        
        return -1;
    }
    
    // Get the PATH environment variable
    const char* env_path = getenv("PATH");
    if (!env_path) {
        return -1;
    }
    
    // Make a copy of the PATH
    char* path_copy = strdup(env_path);
    if (!path_copy) {
        return -1;
    }
    
    // Search for the executable in each directory in the PATH
    char* dir = strtok(path_copy, ":");
    while (dir) {
        // Construct the full path
        snprintf(path, path_size, "%s/%s", dir, name);
        
        // Check if the file exists and is executable
        if (access(path, X_OK) == 0) {
            free(path_copy);
            return 0;
        }
        
        // Try the next directory
        dir = strtok(NULL, ":");
    }
    
    // Free the PATH copy
    free(path_copy);
    
    return -1;
}

static int shell_execute_external(const char* name, int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if the command exists in the PATH
    char path[256];
    if (shell_find_executable(name, path, sizeof(path)) != 0) {
        shell_printf("Error: Command not found: %s\n", name);
        return -1;
    }
    
    // Create a new process
    int pid = fork();
    
    if (pid < 0) {
        // Fork failed
        shell_printf("Error: Failed to create process\n");
        return -1;
    } else if (pid == 0) {
        // Child process
        
        // Execute the command
        execv(path, argv);
        
        // If execv returns, it failed
        shell_printf("Error: Failed to execute command: %s\n", name);
        exit(1);
    } else {
        // Parent process
        
        // Wait for the child process to exit
        int status;
        waitpid(pid, &status, 0);
        
        // Return the exit code
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return -1;
        }
    }
}

/**
 * Run the shell
 * 
 * @return: 0 on success, -1 on failure
 */
int shell_run(void) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Set the running flag
    shell_running = 1;
    
    // Print the welcome message
    shell_printf("Welcome to NeuroOS Shell\n");
    shell_printf("Type 'help' for a list of commands\n");
    
    // Main loop
    while (shell_running) {
        // Print the prompt
        shell_printf("%s", shell_prompt);
        
        // Read a command
        char command[SHELL_MAX_COMMAND_LENGTH];
        
        if (shell_gets(command, sizeof(command)) < 0) {
            break;
        }
        
        // Execute the command
        shell_execute_command(command);
    }
    
    // Print the goodbye message
    shell_printf("Goodbye!\n");
    
    return 0;
}

/**
 * Help command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_help(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Print the help message
    shell_printf("NeuroOS Shell Commands:\n");
    
    for (size_t i = 0; i < shell_command_count; i++) {
        shell_printf("  %-10s %s\n", shell_commands[i].name, shell_commands[i].description);
    }
    
    return 0;
}

/**
 * Exit command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_exit(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Set the running flag
    shell_running = 0;
    
    return 0;
}

/**
 * Echo command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_echo(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Print the arguments
    for (int i = 1; i < argc; i++) {
        shell_printf("%s", argv[i]);
        
        if (i < argc - 1) {
            shell_printf(" ");
        }
    }
    
    shell_printf("\n");
    
    return 0;
}

/**
 * Change directory command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_cd(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if a directory was specified
    if (argc < 2) {
        // Change to the home directory
        const char* home = getenv("HOME");
        if (!home) {
            shell_printf("Error: HOME environment variable not set\n");
            return -1;
        }
        
        if (chdir(home) != 0) {
            shell_printf("Error: Failed to change directory to '%s'\n", home);
            return -1;
        }
    } else {
        // Change to the specified directory
        if (chdir(argv[1]) != 0) {
            shell_printf("Error: Failed to change directory to '%s'\n", argv[1]);
            return -1;
        }
    }
    
    // Update the shell prompt
    char cwd[256];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        snprintf(shell_prompt, sizeof(shell_prompt), "NeuroOS:%s> ", cwd);
    }
    
    return 0;
}

/**
 * Print working directory command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_pwd(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Get the current working directory
    char cwd[256];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        shell_printf("Error: Failed to get current directory\n");
        return -1;
    }
    
    // Print the current working directory
    shell_printf("%s\n", cwd);
    
    return 0;
}

/**
 * List directory contents command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_ls(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Determine the directory to list
    const char* dir_path = ".";
    if (argc > 1) {
        dir_path = argv[1];
    }
    
    // Open the directory
    DIR* dir = opendir(dir_path);
    if (!dir) {
        shell_printf("Error: Failed to open directory '%s'\n", dir_path);
        return -1;
    }
    
    // Read the directory entries
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip hidden files if not requested
        if (entry->d_name[0] == '.' && (argc <= 2 || strcmp(argv[2], "-a") != 0)) {
            continue;
        }
        
        // Get file information
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);
        
        struct stat st;
        if (stat(path, &st) == 0) {
            // Format the file size
            char size_str[16];
            if (S_ISDIR(st.st_mode)) {
                strcpy(size_str, "<DIR>");
            } else {
                if (st.st_size < 1024) {
                    snprintf(size_str, sizeof(size_str), "%ld B", st.st_size);
                } else if (st.st_size < 1024 * 1024) {
                    snprintf(size_str, sizeof(size_str), "%.1f KB", st.st_size / 1024.0);
                } else if (st.st_size < 1024 * 1024 * 1024) {
                    snprintf(size_str, sizeof(size_str), "%.1f MB", st.st_size / (1024.0 * 1024.0));
                } else {
                    snprintf(size_str, sizeof(size_str), "%.1f GB", st.st_size / (1024.0 * 1024.0 * 1024.0));
                }
            }
            
            // Format the file permissions
            char perm_str[11];
            perm_str[0] = S_ISDIR(st.st_mode) ? 'd' : '-';
            perm_str[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
            perm_str[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
            perm_str[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';
            perm_str[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
            perm_str[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
            perm_str[6] = (st.st_mode & S_IXGRP) ? 'x' : '-';
            perm_str[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
            perm_str[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
            perm_str[9] = (st.st_mode & S_IXOTH) ? 'x' : '-';
            perm_str[10] = '\0';
            
            // Format the modification time
            char time_str[20];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", localtime(&st.st_mtime));
            
            // Print the file information
            shell_printf("%s %8s %s %s\n", perm_str, size_str, time_str, entry->d_name);
        } else {
            // Failed to get file information
            shell_printf("???????? ???????? ?????????? %s\n", entry->d_name);
        }
    }
    
    // Close the directory
    closedir(dir);
    
    return 0;
}

/**
 * Concatenate and display files command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_cat(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if a file was specified
    if (argc < 2) {
        shell_printf("Usage: cat <file> [file2] [file3] ...\n");
        return -1;
    }
    
    // Process each file
    int exit_code = 0;
    
    for (int i = 1; i < argc; i++) {
        // Open the file
        FILE* file = fopen(argv[i], "r");
        
        if (!file) {
            shell_printf("Error: Failed to open file '%s'\n", argv[i]);
            exit_code = -1;
            continue;
        }
        
        // Read and print the file contents
        char buffer[1024];
        size_t bytes_read;
        
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            fwrite(buffer, 1, bytes_read, stdout);
        }
        
        // Close the file
        fclose(file);
    }
    
    return exit_code;
}

/**
 * Create directories command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_mkdir(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if a directory was specified
    if (argc < 2) {
        shell_printf("Usage: mkdir <directory> [directory2] [directory3] ...\n");
        return -1;
    }
    
    // Process each directory
    int exit_code = 0;
    
    for (int i = 1; i < argc; i++) {
        // Create the directory
        if (mkdir(argv[i], 0755) != 0) {
            shell_printf("Error: Failed to create directory '%s'\n", argv[i]);
            exit_code = -1;
        }
    }
    
    return exit_code;
}

/**
 * Remove directories command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_rmdir(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if a directory was specified
    if (argc < 2) {
        shell_printf("Usage: rmdir <directory> [directory2] [directory3] ...\n");
        return -1;
    }
    
    // Process each directory
    int exit_code = 0;
    
    for (int i = 1; i < argc; i++) {
        // Remove the directory
        if (rmdir(argv[i]) != 0) {
            shell_printf("Error: Failed to remove directory '%s'\n", argv[i]);
            exit_code = -1;
        }
    }
    
    return exit_code;
}

/**
 * Remove files or directories command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_rm(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if a file was specified
    if (argc < 2) {
        shell_printf("Usage: rm [-r] <file/directory> [file/directory2] [file/directory3] ...\n");
        return -1;
    }
    
    // Check for the recursive flag
    int recursive = 0;
    int start_index = 1;
    
    if (argc > 1 && strcmp(argv[1], "-r") == 0) {
        recursive = 1;
        start_index = 2;
        
        if (argc < 3) {
            shell_printf("Usage: rm -r <file/directory> [file/directory2] [file/directory3] ...\n");
            return -1;
        }
    }
    
    // Process each file/directory
    int exit_code = 0;
    
    for (int i = start_index; i < argc; i++) {
        // Check if it's a directory
        struct stat st;
        
        if (stat(argv[i], &st) == 0 && S_ISDIR(st.st_mode)) {
            if (recursive) {
                // Remove the directory recursively
                char command[512];
                snprintf(command, sizeof(command), "rm -r %s", argv[i]);
                
                if (system(command) != 0) {
                    shell_printf("Error: Failed to remove directory '%s'\n", argv[i]);
                    exit_code = -1;
                }
            } else {
                shell_printf("Error: '%s' is a directory (use -r to remove recursively)\n", argv[i]);
                exit_code = -1;
            }
        } else {
            // Remove the file
            if (unlink(argv[i]) != 0) {
                shell_printf("Error: Failed to remove file '%s'\n", argv[i]);
                exit_code = -1;
            }
        }
    }
    
    return exit_code;
}

/**
 * Copy files or directories command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_cp(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if source and destination were specified
    if (argc < 3) {
        shell_printf("Usage: cp [-r] <source> <destination>\n");
        return -1;
    }
    
    // Check for the recursive flag
    int recursive = 0;
    int start_index = 1;
    
    if (argc > 1 && strcmp(argv[1], "-r") == 0) {
        recursive = 1;
        start_index = 2;
        
        if (argc < 4) {
            shell_printf("Usage: cp -r <source> <destination>\n");
            return -1;
        }
    }
    
    // Get the source and destination
    const char* source = argv[start_index];
    const char* destination = argv[start_index + 1];
    
    // Check if the source exists
    struct stat st;
    
    if (stat(source, &st) != 0) {
        shell_printf("Error: Source '%s' does not exist\n", source);
        return -1;
    }
    
    // Check if the source is a directory
    if (S_ISDIR(st.st_mode)) {
        if (recursive) {
            // Copy the directory recursively
            char command[512];
            snprintf(command, sizeof(command), "cp -r %s %s", source, destination);
            
            if (system(command) != 0) {
                shell_printf("Error: Failed to copy directory '%s' to '%s'\n", source, destination);
                return -1;
            }
        } else {
            shell_printf("Error: '%s' is a directory (use -r to copy recursively)\n", source);
            return -1;
        }
    } else {
        // Copy the file
        FILE* src_file = fopen(source, "rb");
        
        if (!src_file) {
            shell_printf("Error: Failed to open source file '%s'\n", source);
            return -1;
        }
        
        FILE* dst_file = fopen(destination, "wb");
        
        if (!dst_file) {
            fclose(src_file);
            shell_printf("Error: Failed to open destination file '%s'\n", destination);
            return -1;
        }
        
        // Copy the file contents
        char buffer[4096];
        size_t bytes_read;
        
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
            if (fwrite(buffer, 1, bytes_read, dst_file) != bytes_read) {
                fclose(src_file);
                fclose(dst_file);
                shell_printf("Error: Failed to write to destination file '%s'\n", destination);
                return -1;
            }
        }
        
        // Close the files
        fclose(src_file);
        fclose(dst_file);
        
        // Copy the file permissions
        chmod(destination, st.st_mode & 0777);
    }
    
    return 0;
}

/**
 * Move or rename files or directories command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_mv(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if source and destination were specified
    if (argc < 3) {
        shell_printf("Usage: mv <source> <destination>\n");
        return -1;
    }
    
    // Get the source and destination
    const char* source = argv[1];
    const char* destination = argv[2];
    
    // Check if the source exists
    struct stat st;
    
    if (stat(source, &st) != 0) {
        shell_printf("Error: Source '%s' does not exist\n", source);
        return -1;
    }
    
    // Move/rename the file or directory
    if (rename(source, destination) != 0) {
        shell_printf("Error: Failed to move/rename '%s' to '%s'\n", source, destination);
        return -1;
    }
    
    return 0;
}

/**
 * Change file timestamps command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_touch(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if a file was specified
    if (argc < 2) {
        shell_printf("Usage: touch <file> [file2] [file3] ...\n");
        return -1;
    }
    
    // Process each file
    int exit_code = 0;
    
    for (int i = 1; i < argc; i++) {
        // Check if the file exists
        FILE* file = fopen(argv[i], "r");
        
        if (file) {
            // File exists, close it
            fclose(file);
            
            // Update the file's timestamp
            if (utimes(argv[i], NULL) != 0) {
                shell_printf("Error: Failed to update timestamp for file '%s'\n", argv[i]);
                exit_code = -1;
            }
        } else {
            // File doesn't exist, create it
            file = fopen(argv[i], "w");
            
            if (!file) {
                shell_printf("Error: Failed to create file '%s'\n", argv[i]);
                exit_code = -1;
                continue;
            }
            
            // Close the file
            fclose(file);
        }
    }
    
    return exit_code;
}

/**
 * Search for patterns in files command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_grep(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if a pattern and file were specified
    if (argc < 3) {
        shell_printf("Usage: grep <pattern> <file> [file2] [file3] ...\n");
        return -1;
    }
    
    // Get the pattern
    const char* pattern = argv[1];
    
    // Process each file
    int exit_code = 0;
    int match_found = 0;
    
    for (int i = 2; i < argc; i++) {
        // Open the file
        FILE* file = fopen(argv[i], "r");
        
        if (!file) {
            shell_printf("Error: Failed to open file '%s'\n", argv[i]);
            exit_code = -1;
            continue;
        }
        
        // Read and search each line
        char line[1024];
        int line_number = 0;
        
        while (fgets(line, sizeof(line), file)) {
            line_number++;
            
            // Remove the trailing newline
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
                len--;
            }
            
            // Search for the pattern
            if (strstr(line, pattern)) {
                // Print the match
                if (argc > 3) {
                    shell_printf("%s:%d: %s\n", argv[i], line_number, line);
                } else {
                    shell_printf("%d: %s\n", line_number, line);
                }
                
                match_found = 1;
            }
        }
        
        // Close the file
        fclose(file);
    }
    
    // Return success if at least one match was found
    return match_found ? 0 : 1;
}

/**
 * Search for files in a directory hierarchy command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_find(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if a directory was specified
    if (argc < 2) {
        shell_printf("Usage: find <directory> [-name <pattern>] [-type f|d]\n");
        return -1;
    }
    
    // Get the directory
    const char* directory = argv[1];
    
    // Check if the directory exists
    struct stat st;
    
    if (stat(directory, &st) != 0 || !S_ISDIR(st.st_mode)) {
        shell_printf("Error: '%s' is not a directory\n", directory);
        return -1;
    }
    
    // Parse the options
    const char* name_pattern = NULL;
    char type = 0; // 'f' for files, 'd' for directories, 0 for both
    
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-name") == 0 && i + 1 < argc) {
            name_pattern = argv[++i];
        } else if (strcmp(argv[i], "-type") == 0 && i + 1 < argc) {
            if (strcmp(argv[i + 1], "f") == 0) {
                type = 'f';
            } else if (strcmp(argv[i + 1], "d") == 0) {
                type = 'd';
            } else {
                shell_printf("Error: Invalid type '%s'\n", argv[i + 1]);
                return -1;
            }
            
            i++;
        } else {
            shell_printf("Error: Invalid option '%s'\n", argv[i]);
            return -1;
        }
    }
    
    // Find the files
    char command[512];
    
    if (name_pattern && type) {
        snprintf(command, sizeof(command), "find %s -name \"%s\" -type %c", directory, name_pattern, type);
    } else if (name_pattern) {
        snprintf(command, sizeof(command), "find %s -name \"%s\"", directory, name_pattern);
    } else if (type) {
        snprintf(command, sizeof(command), "find %s -type %c", directory, type);
    } else {
        snprintf(command, sizeof(command), "find %s", directory);
    }
    
    // Execute the command
    return system(command) == 0 ? 0 : -1;
}

/**
 * Display command history command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_history(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Get the history
    char* history[SHELL_MAX_HISTORY];
    size_t count = 0;
    
    if (shell_get_history(history, &count) != 0) {
        shell_printf("Error: Failed to get command history\n");
        return -1;
    }
    
    // Print the history
    for (size_t i = 0; i < count; i++) {
        shell_printf("%3zu: %s\n", i + 1, history[i]);
    }
    
    return 0;
}

/**
 * Clear the terminal screen command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_clear(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Clear the screen using ANSI escape codes
    shell_printf("\033[2J\033[H");
    
    return 0;
}

/**
 * Display the current date and time command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_date(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Get the current time
    time_t now = time(NULL);
    
    if (now == (time_t)-1) {
        shell_printf("Error: Failed to get current time\n");
        return -1;
    }
    
    // Format the time
    struct tm* tm_info = localtime(&now);
    
    if (!tm_info) {
        shell_printf("Error: Failed to convert time\n");
        return -1;
    }
    
    char buffer[64];
    
    // Format: Day Mon DD HH:MM:SS YYYY
    strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", tm_info);
    
    // Print the date and time
    shell_printf("%s\n", buffer);
    
    return 0;
}

/**
 * Report process status command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_ps(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Print the header
    shell_printf("  PID  PPID  CMD\n");
    
    // Open the /proc directory
    DIR* proc_dir = opendir("/proc");
    
    if (!proc_dir) {
        shell_printf("Error: Failed to open /proc directory\n");
        return -1;
    }
    
    // Read each entry in the /proc directory
    struct dirent* entry;
    
    while ((entry = readdir(proc_dir)) != NULL) {
        // Skip non-numeric entries
        if (!isdigit(entry->d_name[0])) {
            continue;
        }
        
        // Get the process ID
        pid_t pid = atoi(entry->d_name);
        
        // Open the process status file
        char status_path[64];
        snprintf(status_path, sizeof(status_path), "/proc/%d/status", pid);
        
        FILE* status_file = fopen(status_path, "r");
        
        if (!status_file) {
            continue;
        }
        
        // Read the process status
        char line[256];
        pid_t ppid = 0;
        char name[256] = "";
        
        while (fgets(line, sizeof(line), status_file)) {
            if (strncmp(line, "Name:", 5) == 0) {
                sscanf(line, "Name: %255s", name);
            } else if (strncmp(line, "PPid:", 5) == 0) {
                sscanf(line, "PPid: %d", &ppid);
            }
        }
        
        // Close the status file
        fclose(status_file);
        
        // Print the process information
        shell_printf("%5d %5d  %s\n", pid, ppid, name);
    }
    
    // Close the /proc directory
    closedir(proc_dir);
    
    return 0;
}

/**
 * Send a signal to a process command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_kill(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if a process ID was specified
    if (argc < 2) {
        shell_printf("Usage: kill [-<signal>] <pid>\n");
        return -1;
    }
    
    // Parse the arguments
    int signal = 15; // SIGTERM
    int pid_index = 1;
    
    if (argc > 2 && argv[1][0] == '-') {
        // Parse the signal
        if (isdigit(argv[1][1])) {
            signal = atoi(argv[1] + 1);
        } else if (strcmp(argv[1], "-TERM") == 0 || strcmp(argv[1], "-15") == 0) {
            signal = 15; // SIGTERM
        } else if (strcmp(argv[1], "-KILL") == 0 || strcmp(argv[1], "-9") == 0) {
            signal = 9; // SIGKILL
        } else if (strcmp(argv[1], "-INT") == 0 || strcmp(argv[1], "-2") == 0) {
            signal = 2; // SIGINT
        } else if (strcmp(argv[1], "-HUP") == 0 || strcmp(argv[1], "-1") == 0) {
            signal = 1; // SIGHUP
        } else if (strcmp(argv[1], "-QUIT") == 0 || strcmp(argv[1], "-3") == 0) {
            signal = 3; // SIGQUIT
        } else if (strcmp(argv[1], "-USR1") == 0 || strcmp(argv[1], "-10") == 0) {
            signal = 10; // SIGUSR1
        } else if (strcmp(argv[1], "-USR2") == 0 || strcmp(argv[1], "-12") == 0) {
            signal = 12; // SIGUSR2
        } else {
            shell_printf("Error: Invalid signal '%s'\n", argv[1]);
            return -1;
        }
        
        pid_index = 2;
    }
    
    // Get the process ID
    if (pid_index >= argc) {
        shell_printf("Error: No process ID specified\n");
        return -1;
    }
    
    pid_t pid = atoi(argv[pid_index]);
    
    if (pid <= 0) {
        shell_printf("Error: Invalid process ID '%s'\n", argv[pid_index]);
        return -1;
    }
    
    // Send the signal
    if (kill(pid, signal) != 0) {
        shell_printf("Error: Failed to send signal %d to process %d\n", signal, pid);
        return -1;
    }
    
    return 0;
}

/**
 * Execute a command command
 * 
 * @param argc: Argument count
 * @param argv: Argument array
 * @return: Command exit code
 */
int shell_cmd_exec(int argc, char** argv) {
    // Check if the Shell is initialized
    if (!shell_initialized) {
        return -1;
    }
    
    // Check if a command was specified
    if (argc < 2) {
        shell_printf("Usage: exec <command> [arg1] [arg2] ...\n");
        return -1;
    }
    
    // Find the executable
    char path[256];
    
    if (shell_find_executable(argv[1], path, sizeof(path)) != 0) {
        shell_printf("Error: Command not found: %s\n", argv[1]);
        return -1;
    }
    
    // Execute the command
    execv(path, &argv[1]);
    
    // If execv returns, it failed
    shell_printf("Error: Failed to execute command: %s\n", argv[1]);
    return -1;
}
