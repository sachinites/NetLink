
/* Find my other courses on : http://csepracticals.wixsite.com/csepracticals
 * at great discounts and offers */

/* This Demo of Linux Kernel Module written for 
 * kernel 4.4.0-31-generic*/

/* #include Minimum required header files to write a kernel
 * Module*/

/*This kernel Module Demonstrates the Multicast Messaging from Kernel
 * land to userland*/

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

/*Define a Multicast Group ID*/
#define MCAST_GROUP_ID  16

/*Function that would dump the contents of Netlink Msg
 * recvd*/

static char*
netlink_get_msg_type(__u16 nlmsg_type){

    switch(nlmsg_type){
        case NLMSG_NOOP:
            return "NLMSG_NOOP";
        case NLMSG_ERROR:
            return "NLMSG_ERROR";
        case NLMSG_DONE:
            return "NLMSG_DONE";
        case NLMSG_OVERRUN:
            return "NLMSG_OVERRUN";
        default:
            return "NLMSG_UNKNOWN";
    }
}


static void
nlmsg_dump(struct nlmsghdr *nlh){

    printk(KERN_INFO "Dumping Netlink Msgs Hdr");
    printk(KERN_INFO "  Netlink Msg Type = %s", netlink_get_msg_type(nlh->nlmsg_type));
    printk(KERN_INFO "  Netlink Msg len  = %d", nlh->nlmsg_len);
    printk(KERN_INFO "  Netlink Msg flags  = %d", nlh->nlmsg_flags);  
    printk(KERN_INFO "  Netlink Msg Seq#  = %d", nlh->nlmsg_seq);
    printk(KERN_INFO "  Netlink Msg Pid#  = %d", nlh->nlmsg_pid);
}


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

/* When the Data/msg comes over netlink socket from userspace, kernel
 * packages the data in sk_buff data structures and invokes the below
 * function with pointer to that skb*/

static void netlink_recv_msg_fn(struct sk_buff *skb_in){

    struct nlmsghdr *nlh;
    int user_space_data_len;
    struct sk_buff *skb_out = NULL;
    int res;
    int reply_msg_size = 32;

    /*free the memory occupied by skb*/
    printk(KERN_INFO "%s() invoked", __FUNCTION__);

    /*skb carries Netlink Msg which starts with Netlink Header*/
    nlh = (struct nlmsghdr*)(skb_in->data);
    
    printk(KERN_INFO "%s(%d) : skb_in = %p, nlh = %p, skb_in->len = %u\n", 
        __FUNCTION__, __LINE__, skb_in, nlh, skb_in->len);
   
    user_space_data_len = skb_in->len;
   
    /*Loop over all Cadcaded Netlink Headers*/ 
    for(nlh; 
        ((NLMSG_OK(nlh, user_space_data_len)) && 
        ((nlh->nlmsg_flags & NLM_F_MULTI) || 
         nlh->nlmsg_type == NLMSG_DONE)); 
        nlh = NLMSG_NEXT(nlh, user_space_data_len)) {
        
        nlmsg_dump(nlh);
        printk(KERN_INFO "\nNetlink Data = %s\n", (char *)(NLMSG_DATA(nlh)));
        
        /*Netlink msg type set to NLMSG_DONE should the last Netlink
         * message in the unified buffer*/ 
        if(nlh->nlmsg_type == NLMSG_DONE)
            break;
    }

    /*Sending mcast reply to processes*/
    pr_info("Sending mcast reply to processes with grp id = %d\n", MCAST_GROUP_ID);

    skb_out = nlmsg_new(NLMSG_ALIGN(reply_msg_size + 1), GFP_KERNEL);
    if(!skb_out){
        pr_err("skb_out Allocation failure.\n");
        return;
    }

    nlh = nlmsg_put(skb_out, 0, 1, NLMSG_DONE, reply_msg_size + 1, 0);
    strcpy(nlmsg_data(nlh), "This is Multicast reply\0");

    /*API to send Multicast reply to all processes who have joined group
     * MCAST_GROUP_ID*/
    res = nlmsg_multicast(nl_sk, skb_out, 0, MCAST_GROUP_ID, GFP_KERNEL);
    if (res < 0)
        pr_info("nlmsg_multicast() error: %d\n", res);
    else
        pr_info("Success.\n");

    /* Free the Resources if any, no need to free skb_out, nlmsg_multicast consumes
     * skb_out itself */
}
                     
                     
                     
static struct netlink_kernel_cfg cfg = {
    .input = netlink_recv_msg_fn, /*This fn would recieve msgs from userspace for
                                    Netlink protocol no 31*/
    .groups = 0,                   /*This Kernel Module has not joined any multicast group*/
    .flags = 0,
};                   
                     
/*Init function of this kernel Module*/
static int __init NetlinkMultiCasts_init(void) {
    
    /* All printk output would appear in /var/log/kern.log file
     * use cmd ->  tail -f /var/log/kern.log in separate terminal 
     * window to see output*/
	printk(KERN_INFO "Hello Kernel, I am kernel Module NetlinkMultiCastsLKM.ko\n");
   
    /* Now Create a Netlink Socket in kernel space*/
    /* Arguments : 
     * Network Namespace : Read here : 
     *                     https://blogs.igalia.com/dpino/2016/04/10/network-namespaces
     * Netlink Protocol ID : NETLINK_TEST_PROTOCOL 
     * Netlink Socket Configuration Data 
     * */ 
     
     cfg.flags |= NL_CFG_F_NONROOT_RECV;    /*Allow a non-root user space process to bind to a multicast Group and hence become eligible to recv multicast msgs*/
     cfg.flags |= NL_CFG_F_NONROOT_SEND;    /*Allow a non-root user space process to send multicast msgs*/

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
static void __exit NetlinkMultiCasts_exit(void) {

	printk(KERN_INFO "Bye Bye. Exiting kernel Module NetlinkMultiCastsLKM.ko \n");
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
module_init(NetlinkMultiCasts_init); 

/* Registration of Kernel Module Exit Function.
 * Whenever the Module is removed from kernel using
 * rmmod cmd, below function is triggered. You can do
 * cleanup in this function.*/
module_exit(NetlinkMultiCasts_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Abhishek Sagar");
