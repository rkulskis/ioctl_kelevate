obj-m = kelevate_ioc.o
all: print_cr3
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
print_cr3: print_cr3.c
	$(CC) $^ -o $@
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf *~ print_cr3
