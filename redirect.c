#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <inp> <cmd> <out>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *inp = argv[1];
    char *cmd = argv[2];
    char *out = argv[3];

    // Split cmd into arguments
    char *cmd_args[256]; // This should be enough for most simple commands
    int arg_index = 0;
    char *token = strtok(cmd, " ");
    while (token != NULL) {
        cmd_args[arg_index++] = token;
        token = strtok(NULL, " ");
    }
    cmd_args[arg_index] = NULL; // execvp expects NULL-terminated array

    // Determine absolute path if cmd is not an absolute path
    if (cmd_args[0][0] != '/') {
        char *path = getenv("PATH");
        char abs_path[512];
        char *dir = strtok(path, ":");
        while (dir != NULL) {
            snprintf(abs_path, sizeof(abs_path), "%s/%s", dir, cmd_args[0]);
            if (access(abs_path, X_OK) == 0) {
                cmd_args[0] = abs_path;
                break;
            }
            dir = strtok(NULL, ":");
        }
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Child process
        // Redirect input if necessary
        if (strcmp(inp, "-") != 0) {
            int fd_in = open(inp, O_RDONLY);
            if (fd_in == -1) {
                perror("open input file");
                exit(EXIT_FAILURE);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }

        // Redirect output if necessary
        if (strcmp(out, "-") != 0) {
            int fd_out = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out == -1) {
                perror("open output file");
                exit(EXIT_FAILURE);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }

        // Execute the command
        execvp(cmd_args[0], cmd_args);
        perror("execvp"); // Only reached if exec fails
        exit(EXIT_FAILURE);
    } else { // Parent process
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            fprintf(stderr, "Child process did not exit normally\n");
            return EXIT_FAILURE;
        }
    }
}