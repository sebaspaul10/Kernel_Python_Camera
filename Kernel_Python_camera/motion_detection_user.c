#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define DEVICE_PATH "/dev/motion_device"

int main() {
    int fd;
    char buffer[32];

    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return EXIT_FAILURE;
    }

    while (1) {
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read < 0) {
            perror("Failed to read from device");
            close(fd);
            return EXIT_FAILURE;
        }

        buffer[bytes_read] = '\0'; // Null-terminate the string
        printf("Kernel says: %s", buffer);

        sleep(1); // Poll every second
    }

    close(fd);
    return EXIT_SUCCESS;
}
