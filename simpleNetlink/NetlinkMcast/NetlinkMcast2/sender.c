
 /* Find my other courses on : http://csepracticals.wixsite.com/csepracticals
 * at great discounts and offers */

/*This program demonstate how user-space program can send a msg to 
 * Linux kernel subsystem*/

#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>   /* To use 'errno' as explained in code*/
#include <unistd.h>  /* for getpid() to get process id */

typedef enum {

    FALSE = 0,
    TRUE
} boolean;

#define NETLINK_TEST_PROTOCOL 31   /* user define netlink protocol.
                                      It should match with Netlink Socket protocol opened in kernel space.*/

#define MAX_PAYLOAD 1024            /* maximum payload size sent from Userspace to Kernel Space*/

int main() {

    int rc = 0;
     
    struct sockaddr_nl dest_addr;

    /* Netlink msghdr for sending and recieving msgs*/
    struct nlmsghdr *nlh = NULL, 
                    *nlh_temp = NULL;

    struct iovec iov;             /* iovector - will explain later, for now just understand it is a conatiner of nlmsghdr*/
                                  /* So, notice we go through the example, iovec is a container of nlmsghdr, struct msghdr is 
                                   * a container of iovec. I will explain why it is done this way, there are benefits of doing
                                   * this way*/

    static struct msghdr msg;           /* Outermost msg sturucture which will be a container of iovec, iovec inturn
                                            is a container of nlmsghdr */

    /* Create a net link socket using usual socket() system call*/
    /* When SOCK_RAW is used, Application has to pepare the struct msghdr structure and send msg using sendmsg().
     * When SOCK_DGRAM is used, socket layer will take care to prepare struct msghdr for you. You have to use sendto() in this case.
     * We shall demonstrate both the cases. In this file, I have demonstrated SOCK_RAW case */

    int sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_TEST_PROTOCOL);

    if(sock_fd == -1){
        printf("Error : Netlink socket creation failed\n");
        exit(1);
    }

    /* The application needs to specify whom it is sending the msg over Netlink Protocol.
     * In this case, our application is interested in sending msg to kernel (Other options are : Other userspace
     * applicationis).
     * Our application needs to specify the destination address - The kernel's address using dest-pid = 0.
     * In kernel, any kernel subsystem/module which has
     * opened the Netlink socket for protocol NETLINK_TEST_PROTOCOL as Netlink protocol
     * will going to recieve this msg. */


    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;    /* For Linux Kernel this is always Zero*/
    dest_addr.nl_groups = 0; /* Sending to Linux Kernel subsystem who has not joined any multicast group*/


    /* now, we need to send a Netlink msg to Linux kernel Module.
     * We need to take a space for Netlink Msg Hdr followed by payload msg.
     */
    nlh=(struct nlmsghdr *)calloc(3,
            NLMSG_SPACE(MAX_PAYLOAD) + NLMSG_HDRLEN);   /*Always use the macro NLMSG_SPACE to calculate the size of Payload data. This macro will take care to do all necessary alignment*/

    /* Fill the netlink message header */
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD) + NLMSG_HDRLEN; /* size of the msg including netlink header*/
    nlh->nlmsg_pid = getpid();                                /* Netlink Port id, should be unique to a process, good practice is to use process-id */
    nlh->nlmsg_flags |= NLM_F_MULTI;                          /* More about this Later*/
    nlh->nlmsg_type = 0;
    nlh->nlmsg_seq = 1; 

    /* Fill in the netlink message payload */
    strcpy(NLMSG_DATA(nlh), "Hello you!, This is msg 1 of multipart msg");                  /*Copy the application data to Netlink payload space. Use macro NLMSG_DATA to get ptr to netlink payload data space*/

    nlh_temp = (char *)nlh + nlh->nlmsg_len;
    nlh_temp->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD) + NLMSG_HDRLEN;
    nlh_temp->nlmsg_pid = getpid(); 
    nlh_temp->nlmsg_flags |= NLM_F_MULTI;
    nlh_temp->nlmsg_type = 0;
    nlh_temp->nlmsg_seq = 1;
    strcpy(NLMSG_DATA(nlh_temp), "Hello you!, This is msg 2 of multipart msg");


    nlh_temp = (char *)nlh_temp + nlh_temp->nlmsg_len;
    nlh_temp = (char *)nlh_temp + nlh_temp->nlmsg_len;
    nlh_temp->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD) + NLMSG_HDRLEN;
    nlh_temp->nlmsg_pid = getpid(); 
    nlh_temp->nlmsg_flags = 0;
    nlh_temp->nlmsg_type = NLMSG_DONE;
    nlh_temp->nlmsg_seq = 1;
    strcpy(NLMSG_DATA(nlh_temp), "Hello you!, This is msg 3 of multipart msg");

    /*Now, wrap the data to be send inside iovec*/

    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len * 3;           /*Total length of all Netlink msgs*/

    /*Now wrap the iovec inside the msghdr*/
    memset(&msg, 0, sizeof(struct msghdr));                 /*Always memset msghdr if it is local variable as it may contain garbage values*/
    msg.msg_name = (void *)&dest_addr;                      /* whom you are sending this msg to, the KENEL*/
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;                                     /*We are sending only 1 iovec*/

    rc = sendmsg(sock_fd,&msg,0);           /*Finally send msg to linux kernel*/
    printf("No of bytes send = %d, errno = %u\n", rc, errno);       /*sendmsg return value is no of bytes sent including netlink hdr, errno is set to zero if it succeeds.
                                                                      if sendmsg fails for some reason, after sending a msg, always print errno to find why sendmsg is failed.*/

    /*Now that we have successfully send the msg to Linux kernel, 
     * Now let us recv the reply from kernel*/

    /*Clean up all dynamic memories now*/
    free(nlh);
    
    /*Close the socket before terminating the application*/
    close(sock_fd);
    return 0;
}
