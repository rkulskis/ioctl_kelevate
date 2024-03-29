obj-m = kelevate_ioc.o
all: print_cr3
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
print_cr3: print_cr3.c
	$(CC) -g $^ -o $@
in:
	insmod kelevate_ioc.ko
	mknod /dev/symbi_ioctl c 448 0
	./print_cr3
rm:
	rmmod kelevate_ioc.ko
	rm /dev/symbi_ioctl
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf *~ print_cr3
