/**
 * shell.h - Shell interface for NeuroOS
 *
 * This file contains the shell interface definitions and declarations for
 * the NeuroOS shell module.
 */

#ifndef NEUROOS_SHELL_H
#define NEUROOS_SHELL_H

#include <stddef.h>
#include <stdint.h>

// Shell constants
#define SHELL_MAX_COMMAND_LENGTH 256
#define SHELL_MAX_ARGS 32
#define SHELL_MAX_HISTORY 100

// Shell command function type
typedef int (*shell_command_func_t)(int argc, char** argv);

// Shell command structure
typedef struct {
    const char* name;
    const char* description;
    shell_command_func_t func;
} shell_command_t;

// Shell initialization and shutdown
int shell_init(void);
int shell_shutdown(void);

// Shell command operations
int shell_register_command(const char* name, const char* description, shell_command_func_t func);
int shell_unregister_command(const char* name);
int shell_execute_command(const char* command);
int shell_execute_script(const char* script_path);

// Shell I/O operations
int shell_printf(const char* format, ...);
int shell_gets(char* buffer, size_t size);
int shell_getchar(void);
int shell_putchar(int c);

// Shell history operations
int shell_add_to_history(const char* command);
int shell_clear_history(void);
int shell_get_history(char** history, size_t* count);

// Shell utility functions
int shell_parse_command(const char* command, int* argc, char** argv);

// Shell run function
int shell_run(void);

// Built-in shell commands
int shell_cmd_help(int argc, char** argv);
int shell_cmd_exit(int argc, char** argv);
int shell_cmd_echo(int argc, char** argv);
int shell_cmd_cd(int argc, char** argv);
int shell_cmd_pwd(int argc, char** argv);
int shell_cmd_ls(int argc, char** argv);
int shell_cmd_cat(int argc, char** argv);
int shell_cmd_mkdir(int argc, char** argv);
int shell_cmd_rmdir(int argc, char** argv);
int shell_cmd_rm(int argc, char** argv);
int shell_cmd_cp(int argc, char** argv);
int shell_cmd_mv(int argc, char** argv);
int shell_cmd_touch(int argc, char** argv);
int shell_cmd_grep(int argc, char** argv);
int shell_cmd_find(int argc, char** argv);
int shell_cmd_history(int argc, char** argv);
int shell_cmd_clear(int argc, char** argv);
int shell_cmd_date(int argc, char** argv);
int shell_cmd_ps(int argc, char** argv);
int shell_cmd_kill(int argc, char** argv);
int shell_cmd_exec(int argc, char** argv);

#endif // NEUROOS_SHELL_H
