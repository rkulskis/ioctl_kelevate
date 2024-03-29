#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MAJOR_NUM 448
#define MINOR_NUM 1
#define SYMBI_IOCTL _IOWR(MAJOR_NUM, MINOR_NUM, unsigned long)

int main() {
    int fd;
    unsigned long cr3_value;
    unsigned long arg = 1; // Argument to pass to the ioctl

    // Open the device file
    fd = open("/dev/symbi_ioctl", O_RDWR);
    printf ("before the ioctl call!\n");    
    if (fd < 0) {
        perror("Failed to open the device file");
        return EXIT_FAILURE;
    }

    // Execute ioctl to set GS register
    if (ioctl(fd, SYMBI_IOCTL, &arg) < 0) {
        perror("ioctl ARCH_SET_GS failed");
        close(fd);
        return EXIT_FAILURE;
    }

    printf ("past the ioctl call!\n");
    // Use inline assembly to move the contents of CR3 into a register
    asm volatile("mov %%cr3, %0" : "=r" (cr3_value));

    // Execute ioctl to get GS register
    arg = 0;
    if (ioctl(fd, SYMBI_IOCTL, &arg) < 0) {
        perror("ioctl ARCH_GET_GS failed");
        close(fd);
        return EXIT_FAILURE;
    }

    // Print the value of the register
    printf("CR3 value: 0x%lx\n", cr3_value);

    // Close the device file
    close(fd);

    return EXIT_SUCCESS;
}
