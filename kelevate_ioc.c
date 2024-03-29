#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <linux/percpu.h>
#include <asm/processor.h>

struct SymbiReg {
    union {
        uint64_t raw;
        struct {
            uint64_t elevate : 1;
            uint64_t query : 1;
            uint64_t int_disable : 1;
            uint64_t debug : 1;
        };
    };
}__attribute__((packed));

#define MAJOR_NUM 448
#define MINOR_NUM 1
#define DEVICE_NAME "symbi_ioctl"
#define SYMBI_IOCTL _IOWR(MAJOR_NUM, MINOR_NUM, struct SymbiReg)
DEFINE_PER_CPU(unsigned long, cpu_current_top_of_stack);

uint64_t symbi_check_elevate(void);
 uint64_t symbi_check_elevate(){
   return current->symbiote_elevated;
 }

 void symbi_print_user_reg_state(struct pt_regs * regs){
   printk("IP %#lx\n", regs->ip);
   printk("SP %#lx\n", regs->sp);
   printk("CS %#lx\n", regs->cs);
   printk("SS %#lx\n", regs->ss);
   printk("FG %#lx\n", regs->flags);
   printk("\n");
 }

 void symbi_query(struct pt_regs* regs){
   int ret = symbi_check_elevate();
   if(ret & 1){
     regs->ss = 0x18;
     regs->cs = 0x10;
   }else{
     BUG_ON(regs->cs != 0x33);
     BUG_ON(regs->ss != 0x2b);
   }
 }

 void symbi_lower(struct pt_regs* regs, struct SymbiReg* sreg){
   if(current->symbiote_elevated == 1 ){
     current->symbiote_elevated = 0;
   } else{
     printk("Trying to lower non elevated task???\n");
   }

   // User interrupts better be enabled....
   if( (regs->flags & (1<<9)) == 0){
     if(sreg->debug){
       printk("Warning: attempted lowering with user interrupts disabled... enabling!");
     }
     regs->flags = regs->flags | (1<<9);
   }
   // Established at syscall entry.
   BUG_ON(regs->cs != 0x33);
   BUG_ON(regs->ss != 0x2b);
 }

 void symbi_elevate(struct pt_regs* regs, struct SymbiReg* sreg){
   // Swing symbiote reg
   if(current->symbiote_elevated == 1 ){
     printk("Already Elevated???\n");
   } else{
     /* printk("setting elevated\n"); */
     current->symbiote_elevated = 1;
   }

   // Modify stack memory used for iret. // x86 specific
   regs->ss = 0x18;
   regs->cs = 0x10;

   // Disable interrupts for user
   if(sreg->int_disable){
     /* printk("setting int disabled\n"); */
     regs->flags= regs->flags & (~(1<<9)); // x86 specific
   }
 }

 void symbi_debug_entry(struct pt_regs *regs, struct SymbiReg *sreg){
   printk("Elevate Syscall Case: ");
   // What case are we in?
   if(sreg->query){
     printk("Query\n");
   } else if(sreg->elevate){
     printk("Elevate\n");
   } else if(!sreg->elevate){
     printk("Lower\n");
   } else{
     printk("Error\n");
   }

   printk("Was user elevated? %llx", symbi_check_elevate());
   printk("Syscall passed flags: %#llx\n", sreg->raw);
   printk("db: %x id: %x q: %x e: %x\n", sreg->debug, sreg->int_disable, sreg->query, sreg->elevate);

   printk("User reg state inbound\n");
   symbi_print_user_reg_state(regs);
 }

static long int symbi_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct SymbiReg sreg;
    struct pt_regs *regs;
    unsigned long *stack_pointer;

    switch (cmd) {
        case SYMBI_IOCTL:
            if (copy_from_user(&sreg.raw, (void __user *)arg, sizeof(unsigned long)))
                return -EFAULT;

	    stack_pointer = (unsigned long *)get_cpu_var(cpu_current_top_of_stack);

            regs = (struct pt_regs *)this_cpu_read(stack_pointer) - 1;
            if (sreg.debug) {
                symbi_debug_entry(regs, &sreg);
            }

            if (sreg.query) {
                symbi_query(regs);
            } else if (sreg.elevate) {
                symbi_elevate(regs, &sreg);
            } else if (!sreg.elevate) {
                symbi_lower(regs, &sreg);
            } else {
                printk("Elevation error: Unexpected input %llx\n", sreg.raw);
                symbi_print_user_reg_state(regs);
                return -EINVAL;
            }

            if (sreg.debug) {
                printk("Elevate bit now %llx\n", symbi_check_elevate());
                symbi_print_user_reg_state(regs);
                printk("About to return from elevate syscall\n");
            }
            if (copy_to_user((void __user *)arg, &sreg.raw, sizeof(unsigned long)))
                return -EFAULT;
            return symbi_check_elevate();
        default:
            return -ENOTTY;
    }
}

static const struct file_operations symbi_fops = {
    .unlocked_ioctl = symbi_ioctl,
};

static int __init symbi_init(void)
{
    int ret;

    ret = register_chrdev(MAJOR_NUM, DEVICE_NAME, &symbi_fops);
    if (ret < 0) {
        printk(KERN_ERR "Failed to register character device\n");
        return ret;
    }

    printk(KERN_INFO "symbi_ioctl module loaded\n");
    printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, 448);    
    return 0;
}

static void __exit symbi_exit(void)
{
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    printk(KERN_INFO "symbi_ioctl module unloaded\n");
}

module_init(symbi_init);
module_exit(symbi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ross Mikulskis");
MODULE_DESCRIPTION("kElevate syscall as an ioctl");
MODULE_VERSION("0.1");
