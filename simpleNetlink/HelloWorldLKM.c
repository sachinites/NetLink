
/* Demo Hello World Linux Kernel Module written for 
 * kernel 4.4.0-31-generic*/

/* #include Minimum required header files to write a kernel
 * Module*/

#include <linux/module.h>   /*Include this for any kernel Module you write*/
#include <linux/netlink.h>  /*Use it for Netlink functionality*/

/*Init function of this kernel Module*/
static int __init HelloWorld_init(void) {
    
    /* All printk output would appear in /var/log/kern.log file
     * use cmd ->  tail -f /var/log/kern.log in separate terminal 
     * window to see output*/
	printk(KERN_INFO "Hello Kernel, I am kernel Module HelloWorldLKM.ko\n");


    /*This fn must return 0 for module to successfully make its way into kernel*/
	return 0;
}

/*Exit function of this kernel Module*/
static void __exit HelloWorld_exit(void) {

	printk(KERN_INFO "Bye Bye. Exiting kernel Module HelloWorldLKM.ko \n");
    /*Release any kernel resources held by this module in this fn*/
}


/*Every Linux Kernel Module has Init and Exit functions - just
 * like normal C program has main() fn.*/

/* Registration of Kernel Module Init Function.
 * Whenever the Module is inserted into kernel using
 * insmod cmd, below function is triggered. You can do
 * all initializations here*/
module_init(HelloWorld_init); 

/* Registration of Kernel Module Exit Function.
 * Whenever the Module is removed from kernel using
 * rmmod cmd, below function is triggered. You can do
 * cleanup in this function.*/
module_exit(HelloWorld_exit);

MODULE_LICENSE("GPL");
