
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
#define MCAST_GRP_ID    16

int main() {

    int rc = 0;
     
    struct sockaddr_nl src_addr;

    /* Netlink msghdr for recieving msgs*/
    struct nlmsghdr *nlh_recv = NULL;

    struct iovec iov;             /* iovector - will explain later, for now just understand it is a conatiner of nlmsghdr*/
                                  /* So, notice we go through the example, iovec is a container of nlmsghdr, struct msghdr is 
                                   * a container of iovec. I will explain why it is done this way, there are benefits of doing
                                   * this way*/

    static struct msghdr recv_msg;    /* Outermost msg sturucture which will be a container of iovec, iovec inturn
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
    src_addr.nl_pid = getpid();             /* ID of the application, it should be unique to a process, good pratice to use process-id*/
    src_addr.nl_groups = MCAST_GRP_ID;      /* This process is joining the multicast group MCAST_GRP_ID*/

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

    /*An alternate way of joining the group*/
    int group = MCAST_GRP_ID;

    if (setsockopt(sock_fd, 270, NETLINK_ADD_MEMBERSHIP, &group, sizeof(group)) < 0) {
        printf("Mcast membership join failed : setsockopt < 0\n");
        return -1;
    }

    /* Now let us recv the reply from kernel*/

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
    /*Clean up all dynamic memories now*/
    free(nlh_recv);

    /*Close the socket before terminating the application*/
    close(sock_fd);
    return 0;
}
