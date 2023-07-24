## AVM Challenge

This repository is part of a "challenge" that avm issued to me.


### hello_world
In hello_world folder you can find a hello world device driver program.

### prototype
The Prototype folder contains the actual code that solves the challenge. The file store_message.c is the device driver code, and user.c is the user space program intended to test the store_message.c device driver. The kernel code was tested on a virtual machine running the 5.4.0-150-generic kernel version (a Linux kernel modified by Canonical). As required by the challenge, the device driver provides an interface in which user-space programs can store and read text data. This text data is stored in a doubly-linked list, with a maximum length of 100, which means it can store up to 100 words. Additionally, the driver logs the content of the doubly-linked list, with one word per second logged into the kernel log (/var/log/kern.log). Finally, this device driver is thread safe, thus several processes can call the interface concurrently. The device driver provides the following interface.

read(int fd, void *buf, size_t count)
- fd - the file descriptor of the device.
- buf - the buffer where the words are intended to be stored.
- count - the number of words to be read from the doubly-linked list (kernel space).

write(int fd, const void buf[.count], size_t count)
- fd - the file descriptor of the device.
- buf - the word that should be stored in the doubly-linked list (kernel space).
- count - the length of the word (in bytes, including the null character).