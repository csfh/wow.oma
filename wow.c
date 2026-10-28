/* Sandboxed side of the Bundle. It pokes the daemon and exits. The game is not in
 * this process's PID namespace, so the exit does not stop it. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_NAME "wow/launch.sock"

int main(void) {
    const char *runtime = getenv("XDG_RUNTIME_DIR");
    if (runtime == NULL || runtime[0] == '\0') {
        fprintf(stderr, "wow: XDG_RUNTIME_DIR is not set\n");
        return 1;
    }

    char path[sizeof(((struct sockaddr_un *)0)->sun_path)];
    int wrote = snprintf(path, sizeof path, "%s/%s", runtime, SOCKET_NAME);
    if (wrote < 0 || (size_t)wrote >= sizeof path) {
        fprintf(stderr, "wow: socket path does not fit\n");
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof addr);
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, path, (size_t)wrote + 1);

    for (int attempt = 0; attempt < 50; attempt++) {
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) {
            perror("wow: socket");
            return 1;
        }
        if (connect(fd, (struct sockaddr *)&addr, sizeof addr) == 0) {
            struct timeval wait = {.tv_sec = 5, .tv_usec = 0};
            setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &wait, sizeof wait);
            char reply = 0;
            ssize_t got = read(fd, &reply, 1);
            close(fd);
            if (got == 1 && reply == '1') {
                return 0;
            }
            fprintf(stderr, "wow: launcher did not start\n");
            return 1;
        }
        close(fd);
        usleep(100000);
    }

    fprintf(stderr, "wow: %s did not answer\n", path);
    return 1;
}
