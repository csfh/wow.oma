/* The one program that runs outside the sandbox. It stays up and, on each connection,
 * starts the WoW launcher unless that launch is still running. The sandboxed program
 * only connects here: Proton, gamescope and the game prefix are outside a Bundle's grants. */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define LAUNCHER "/home/hallas/Launchers/wow.sh"

static pid_t child_pid = -1;

static int still_running(void) {
    int status;
    if (child_pid <= 0) {
        return 0;
    }
    pid_t got = waitpid(child_pid, &status, WNOHANG);
    if (got == child_pid || (got < 0 && errno == ECHILD)) {
        child_pid = -1;
        return 0;
    }
    return 1;
}

static int start_launcher(const char *script) {
    if (still_running()) {
        return 1;
    }
    if (access(script, X_OK) != 0) {
        fprintf(stderr, "wowd: cannot run %s: %s\n", script, strerror(errno));
        return 0;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("wowd: fork");
        return 0;
    }
    if (pid == 0) {
        unsetenv("LISTEN_FDS");
        unsetenv("LISTEN_PID");
        unsetenv("LISTEN_FDNAMES");
        execl(script, script, (char *)NULL);
        fprintf(stderr, "wowd: exec %s: %s\n", script, strerror(errno));
        _exit(127);
    }
    child_pid = pid;
    fprintf(stderr, "wowd: started %s\n", script);
    return 1;
}

static int listen_fd(void) {
    const char *pid_text = getenv("LISTEN_PID");
    const char *fds_text = getenv("LISTEN_FDS");
    if (pid_text == NULL || fds_text == NULL) {
        return -1;
    }
    if ((pid_t)atoi(pid_text) != getpid() || atoi(fds_text) < 1) {
        return -1;
    }
    int fd = 3;
    int flags = fcntl(fd, F_GETFL);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
    }
    fcntl(fd, F_SETFD, FD_CLOEXEC);
    return fd;
}

int main(int argc, char **argv) {
    const char *script = argc >= 2 ? argv[1] : LAUNCHER;
    int fd = listen_fd();
    if (fd < 0) {
        fprintf(stderr, "wowd: no systemd socket (LISTEN_PID/LISTEN_FDS)\n");
        return 1;
    }

    for (;;) {
        int client = accept(fd, NULL, NULL);
        if (client < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("wowd: accept");
            return 1;
        }
        fcntl(client, F_SETFD, FD_CLOEXEC);
        char reply = start_launcher(script) ? '1' : '0';
        if (write(client, &reply, 1) != 1) {
            /* The caller went away. The launch, if it happened, still stands. */
        }
        close(client);
    }
}
