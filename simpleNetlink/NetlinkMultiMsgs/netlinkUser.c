
 /* Find my other courses on : http://csepracticals.wixsite.com/csepracticals
 * at great discounts and offers */

/*This program demonstate how user-space program can send multiple
 * Netlink Msgs to kernel module as one single unified msg*/

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
    boolean reply_requested = FALSE;
    /* While sending msg to kernel, we will have to specify src address and dest address
     * in struct sockaddr_nl structure. Src address shall be this application, and Destination
     * adddress shall be kernel */

    struct sockaddr_nl src_addr, dest_addr;

    /* Netlink msghdr for sending and recieving msgs*/
    struct nlmsghdr *nlh = NULL, 
                    *nlh_recv = NULL,
                    *nlh_temp = NULL;

    struct iovec iov;             /* iovector - will explain later, for now just understand it is a conatiner of nlmsghdr*/
                                  /* So, notice we go through the example, iovec is a container of nlmsghdr, struct msghdr is 
                                   * a container of iovec. I will explain why it is done this way, there are benefits of doing
                                   * this way*/

    static struct msghdr msg, recv_msg;    /* Outermost msg sturucture which will be a container of iovec, iovec inturn
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

    memset(&src_addr, 0, sizeof(src_addr));
    
    /* specify who is the sender of the msg (i.e. this application),
     * kenel uses this info to reply back*/

    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();  /* ID of the application, it should be unique to a process, good pratice to use process-id*/
    src_addr.nl_groups = 0;      /* 1:1 communication between this application and kernel, not in mcast groups */

    /* Binding means: here, appln is telling the OS/Kernel that this application (identified using port-id by OS)
     * is interested in receiving the msgs for Netlink protocol# NETLINK_TEST_PROTOCOL.
     * You can see we have specified two arguments in bind(). Kernel will use sock_fd (a handle) to handover the msgs 
     * coming from kernel subsystem (the kernel module we wrote) to deliver to the application whose port-id is 
     * specified in src_address (means, this application itself).
     */

    if(bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr)) == -1){
        printf("Error : Bind has failed\n");
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
    dest_addr.nl_groups = 0; /* unicast 1:1 communication*/


    /* now, we need to send a Netlink msg to Linux kernel Module.
     * We need to take a space for Netlink Msg Hdr followed by payload msg.
     */
    nlh=(struct nlmsghdr *)calloc(3,
            NLMSG_SPACE(MAX_PAYLOAD) + NLMSG_HDRLEN);   /*Always use the macro NLMSG_SPACE to calculate the size of Payload data. This macro will take care to do all necessary alignment*/

    /* Fill the netlink message header */
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD) + NLMSG_HDRLEN; /* size of the msg including netlink header*/
    nlh->nlmsg_pid = getpid();                                /* Netlink Port id, should be unique to a process, good practice is to use process-id */
    nlh->nlmsg_flags |= NLM_F_MULTI;                         /* More about this Later*/
    nlh->nlmsg_type = 0;
    nlh->nlmsg_seq = 1; 

    if(nlh->nlmsg_flags & NLM_F_REQUEST){
        reply_requested = TRUE;
    }
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
    memset(&msg, 0, sizeof(struct msghdr));                  /*Always memset msghdr if it is local variable as it may contain garbage values*/
    msg.msg_name = (void *)&dest_addr;      /* whom you are sending this msg to, the KENEL*/
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;                     /*We are sending only 1 iovec*/

    rc = sendmsg(sock_fd,&msg,0);           /*Finally send msg to linux kernel*/
    printf("No of bytes send = %d, errno = %u\n", rc, errno);       /*sendmsg return value is no of bytes sent including netlink hdr, errno is set to zero if it succeeds.
                                                                      if sendmsg fails for some reason, after sending a msg, always print errno to find why sendmsg is failed.*/

    /*Now that we have successfully send the msg to Linux kernel, 
     * Now let us recv the reply from kernel*/

    if(reply_requested){

        printf("Waiting for message from kernel\n");

        memset(&recv_msg, 0, sizeof(struct msghdr));
        recv_msg.msg_name = NULL;
        recv_msg.msg_namelen = 0;
        nlh_recv = (struct nlmsghdr *)calloc(1, 
                NLMSG_HDRLEN + NLMSG_SPACE(MAX_PAYLOAD));       /*Take a new buffer to recv data from kernel*/
        nlh_recv->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
        nlh_recv->nlmsg_pid = 0;                                /*Since ,we are receiving the data and not sending, pid is not required*/
        nlh_recv->nlmsg_flags = 0;                              /* More about this Later*/

        iov.iov_base = (void *)nlh_recv;
        iov.iov_len = nlh_recv->nlmsg_len; 

        memset(&recv_msg, 0, sizeof(struct msghdr));
        recv_msg.msg_iov = &iov;
        recv_msg.msg_iovlen = 1;

        /* Read message from kernel. Its a blocking system call - Application execuation
         * is suspended at this point and would not resume until it receives linux kernel
         * msg. We can configure recvmsg() to not to block, but lets use it in blocking 
         * mode for now */
        rc = recvmsg(sock_fd, &recv_msg, 0);

        /*We have successfully received reply from linux kernel*/
        /*print the reply msg from kernel. Reply shall be stored in recv_msg.msg_iov->iov_base
         * in same format : that is Netlink hdr followed by payload data*/

        printf("Received message payload: %s, bytes recvd = %d\n", 
                ((char *)(NLMSG_DATA(recv_msg.msg_iov->iov_base))), rc);
    }
    /*Clean up all dynamic memories now*/
    free(nlh);
    
    if(reply_requested)
        free(nlh_recv);

    /*Close the socket before terminating the application*/
    close(sock_fd);
    return 0;
}
