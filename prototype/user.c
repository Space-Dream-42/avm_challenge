#include<stdio.h>
#include <fcntl.h>
#include <unistd.h>
#define DEVICE_FILE "/dev/store-message-dev-0"
#include <stddef.h>
#include <string.h>

char *sample_data_1 = "Oppenheimer";
char *sample_data_2 = "Einstein";
char *sample_data_3 = "Planck";
char *sample_data_4 = "Heisenberg";

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
    rc = write(fd,sample_data_1,strlen(sample_data_1)+1);
    rc = write(fd,sample_data_2,strlen(sample_data_2)+1);
    rc = write(fd,sample_data_3,strlen(sample_data_3)+1);
    rc = write(fd,sample_data_4,strlen(sample_data_4)+1);

    if(rc<0)
    {
        perror("Write opertion of the device failed.\n");
        return -1;
    }

    printf("written bytes=%d,data=%s\n",rc,sample_data_1);

    // now read out the data on the device
    read_bytes = read(fd, read_buff, 4);
    printf("%s\n", read_buff);
    printf("We are here!");

    if(close(fd) < 0)
    {
        perror("Failed to close the device.\n");
        return -1;
    }

    return 0;
}