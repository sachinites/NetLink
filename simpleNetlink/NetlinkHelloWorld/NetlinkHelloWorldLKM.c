
/* Find my other courses on : http://csepracticals.wixsite.com/csepracticals
 * at great discounts and offers */

/* Demo Hello World Linux Kernel Module written for 
 * kernel 4.4.0-31-generic*/

/* #include Minimum required header files to write a kernel
 * Module*/

#include <linux/module.h>   /*Include this for any kernel Module you write*/
#include <linux/netlink.h>  /*Use it for Netlink functionality*/
#include <net/sock.h>       /*Network namespace and socket Based APIs*/
#include <linux/string.h>   /*for memset/memcpy etc..., do not use <string.h>, that is for user space*/
#include <linux/kernel.h>   /*for scnprintf*/


/* Define a New Netlink protocol ID
 * In file linux/netlink.h, you can see in line no [8-33]
 * lists the reserved IDs used for Netlink protocols already implemented
 * in Linux. We can use upto max 31th protocol number for Netlink. Let
 * us just pick the unused one - 31 below */

#define NETLINK_TEST_PROTOCOL   31

/*Global variables of this LKM*/
static struct sock *nl_sk = NULL;       /*Kernel space Netlink socket ptr*/

/* Reciever function for Data received over netlink
 * socket from user space
 * skb - socket buffer, a unified data structiure for 
 * packaging the data type being transported from one kernel
 * subsystem to another or from kernel space to user-space
 * or from user-space to kernel space. The internal member of this
 * socket buffer is not accessed directly, but rather kernel APIs
 * provide setters/getters for this purpose. We will use them as 
 * we would need along the way*/

/*When the Data/msg comes over netlink socket from userspace, kernel
 * packages the data in sk_buff data structures and invokes the below
 * function with pointer to that skb*/
static void netlink_recv_msg_fn(struct sk_buff *skb_in){

    struct nlmsghdr *nlh;
    char *user_space_data;
    int user_space_data_len;
    struct sk_buff *skb_out;
    char kernel_reply[256];
    int user_space_process_port_id;
    int res;

    /*free the memory occupied by skb*/
    printk(KERN_INFO "%s() invoked", __FUNCTION__);

    /*skb carries Netlink Msg which starts with Netlink Header*/
    nlh = (struct nlmsghdr*)(skb_in->data);
    
    printk(KERN_INFO "%s(%d) : skb_in = %p, nlh = %p", __FUNCTION__, __LINE__, skb_in, nlh);

    user_space_process_port_id = nlh->nlmsg_pid;

    printk(KERN_INFO "%s(%d) : port id of the sending user space process = %u", 
            __FUNCTION__, __LINE__, user_space_process_port_id);

    user_space_data = (char*)nlmsg_data(nlh);
    user_space_data_len = nlmsg_len(nlh);
    
    printk(KERN_INFO "%s(%d) : msg recvd from user space= %s, msg_len = %d", 
            __FUNCTION__, __LINE__, user_space_data, user_space_data_len);


    /*Sending reply back to user space process*/
    memset(kernel_reply, 0 , sizeof(kernel_reply));
    
    /*defined in linux/kernel.h */
    snprintf(kernel_reply, sizeof(kernel_reply), 
        "Msg from Process %d has been processed", nlh->nlmsg_pid);
    
    /*Get a new sk_buff with empty Netlink hdr already appended before payload space
     * i.e skb_out->data will be pointer to below msg : 
     *
     * +----------+---------------+
     * |Netlink Hdr|   payload    |
     * ++---------+---------------+
     *
     * */
 
    skb_out = nlmsg_new(sizeof(kernel_reply), 0/*Related to memory allocation, skip...*/);
                                  
    /*Add a TLV*/ 
    nlh = nlmsg_put(skb_out,
            user_space_process_port_id, /*User space port id*/
            0,                          /*Sequence no*/
            NLMSG_DONE,                 /*TLV code point*/
            sizeof(kernel_reply),       /*Payload size*/
            0);                         /*Flags*/
                     
    NETLINK_CB(skb_out).dst_group = 0; /* Kernel is sending this msg to only 1 process*/
                     
    /*copy the paylo d now*/
    strncpy(nlmsg_data(nlh), kernel_reply, sizeof(kernel_reply));
    //skb_get(skb_out ) ;
    //skb_get(skb_out );
                      
    //printk(KERN_IN   "users count of skb_out = %d", atomic_read(&skb_out->users));
    /*Finaly Send the  msg to user space space process*/
    res = nlmsg_unicast(nl_sk, skb_out, user_space_process_port_id);
    //printk(KERN_IN O "users count of skb_out = %d after first unicast", atomic_read(&skb_out->users));
    //res = nlmsg_un cast(nl_sk, skb_out, user_space_process_port_id);
    //printk(KERN_IN O "users count of skb_out = %d after second unicast", atomic_read(&skb_out->users));
    //res = nlmsg_un cast(nl_sk, skb_out, user_space_process_port_id);
                     
    if(res < 0){     
        printk(KERN_INFO "Error while sending the data back to user-space\n");
        kfree_skb(skb_out); /*free the internal skb_data also*/
    }                
    /* Free the Resources if any, no need to free skb_out, nlmsg_unicast consumes
     * skb_out itself */
}
                     
                     
                     
static struct netlink_kernel_cfg cfg = {
    .input = netlink_recv_msg_fn, /*This fn would recieve msgs from userspace for
                                    Netlink protocol no 31*/
    .groups = 0,                  /*For 1:1 Communication between this kernel Module
                                    and User space process*/
    /*There are other parameters of this structure, for now let us
     * not use them as we are just kid !!*/
};                   
                     
/*Init function of this kernel Module*/
static int __init NetlinkHelloWorld_init(void) {
    
    /* All printk output would appear in /var/log/kern.log file
     * use cmd ->  tail -f /var/log/kern.log in separate terminal 
     * window to see output*/
	printk(KERN_INFO "Hello Kernel, I am kernel Module NetlinkHelloWorldLKM.ko\n");
   
    /*Now Create a Netlink Socket in kernel space*/
    /* Arguments : 
     * Network Namespace : Read here : 
     *                     https://blogs.igalia.com/dpino/2016/04/10/network-namespaces
     * Netlink Protocol ID : NETLINK_TEST_PROTOCOL 
     * Netlink Socket Configuration Data 
     * */ 
     
     /*Now create a Netlink socket*/
     nl_sk = netlink_kernel_create(&init_net, NETLINK_TEST_PROTOCOL, &cfg);
     
     if(!nl_sk){
         printk(KERN_INFO "Ketnel Netlink Socket for Netlink protocol %u failed.\n", NETLINK_TEST_PROTOCOL);
         return -ENOMEM; /*All errors are defined in ENOMEM for kernel space, and in stdio.h for user space*/
     }
     
     printk(KERN_INFO "Netlink Socket Created Successfully");
    /*This fn must return 0 for module to successfully make its way into kernel*/
	return 0;
}

/*Exit function of this kernel Module*/
static void __exit NetlinkHelloWorld_exit(void) {

	printk(KERN_INFO "Bye Bye. Exiting kernel Module NetlinkHelloWorldLKM.ko \n");
    /*Release any kernel resources held by this module in this fn*/
    netlink_kernel_release(nl_sk);
    nl_sk = NULL;
}


/*Every Linux Kernel Module has Init and Exit functions - just
 * like normal C program has main() fn.*/

/* Registration of Kernel Module Init Function.
 * Whenever the Module is inserted into kernel using
 * insmod cmd, below function is triggered. You can do
 * all initializations here*/
module_init(NetlinkHelloWorld_init); 

/* Registration of Kernel Module Exit Function.
 * Whenever the Module is removed from kernel using
 * rmmod cmd, below function is triggered. You can do
 * cleanup in this function.*/
module_exit(NetlinkHelloWorld_exit);

MODULE_LICENSE("GPL");
