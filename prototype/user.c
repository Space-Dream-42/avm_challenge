#include<stdio.h>
#include <fcntl.h>
#include <unistd.h>
#define DEVICE_FILE "/dev/cdrv_dev"
#include <stddef.h>
#include <string.h>

char *data = "test data";

int main()
{
    int fd;
    int rc;
    ssize_t read_bytes;
    size_t count;
    count = 256;
    char read_buff[count];


    // Open the device
    fd = open(DEVICE_FILE, O_RDWR);
    if(fd<0)
    {
        perror("Open opertion of the device failed.\n");
        return -1;
    }

    // convey some data to the device
    rc = write(fd,data,strlen(data)+1);

    if(rc<0)
    {
        perror("Write opertion of the device failed.\n");
        return -1;
    }

    printf("written bytes=%d,data=%s\n",rc,data);

    // now read out the data on the device
    read_bytes = read(rc, read_buff, count);

    if(close(fd) < 0)
    {
        perror("Failed to close the device.\n");
        return -1;
    }

    return 0;
}