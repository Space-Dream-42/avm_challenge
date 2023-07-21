KDIR = /usr/src/linux-headers-5.4.0-150-generic

kbuild:
	make -C $(KDIR) M=`pwd` # M= does define a variable in the Makefile. In this case it is the current directory.

clean:
	make -C $(KDIR) M=`pwd` clean