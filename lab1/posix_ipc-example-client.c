#include <stdint.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
//#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>

static char SERVER_PROGRAM_NAME[] = "posix_ipc-example-server";

int main(int argc, char **argv) {
    // Get name of file
    char filename[1024];
    {
        const char prompt[] = "enter filename: ";
        write(STDOUT_FILENO, prompt, sizeof(prompt) - 1);

        ssize_t n = read(STDIN_FILENO, filename, sizeof(filename) - 1);
        if (n <= 0) {
            const char msg[] = "error: failed to read filename\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }
        filename[n] = '\0';
        char *nl = strchr(filename, '\n');
        if (nl) *nl = '\0';
    }

    // Open file
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        const char msg[] = "error: failed to open file\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    // Way to server
    char progpath[1024];
    {
        ssize_t len = readlink("/proc/self/exe", progpath, sizeof(progpath) - 1);
        if (len == -1) { /* Error*/ exit(EXIT_FAILURE); }
        while (progpath[len] != '/') --len;
        progpath[len] = '\0';
    }

    // Open pipe
    int pipe1[2];
    if (pipe(pipe1) == -1) { /* Error */ exit(EXIT_FAILURE); }

    // Spawn a new process
    pid_t child = fork();
    if (child == -1) { /* Error*/ exit(EXIT_FAILURE); }

    if (child == 0) {
        close(pipe1[0]);

        dup2(fd, STDIN_FILENO);
        close(fd);

        dup2(pipe1[1], STDOUT_FILENO);
        close(pipe1[1]);

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", progpath, SERVER_PROGRAM_NAME);
        char *const args[] = {SERVER_PROGRAM_NAME, NULL};
        execv(path, args);
        exit(EXIT_FAILURE);
    }

    close(pipe1[1]);
    close(fd);

    char buf[4096];
    ssize_t bytes;
    while ((bytes = read(pipe1[0], buf, sizeof(buf))) > 0) {
        write(STDOUT_FILENO, buf, bytes);
    }

    close(pipe1[0]);
    wait(NULL);
    return EXIT_SUCCESS;
}