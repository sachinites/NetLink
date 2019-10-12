/*
 *Please check my example program attached to this mail.
 This is very basic and quite buggy example but this source can do the job and you can use it as source of the future inspiration.
 This example program has complex command line interface, here is examples how to use it:

 Add route to 192.168.1.0 using eth0: sudo ./set_route add to 192.168.1.0 dev eth0 
 Delete this route: sudo ./set_route del to 192.168.1.0 dev eth0

 Add route to 192.168.1.0 using eth0 and 192.168.8.1 gateway: sudo ./set_route add to 192.168.8.0 dev eth0 via 192.168.8.1
 Delete this route: sudo ./set_route del to 192.168.8.0 dev eth0 via 192.168.8.1

 To add default gateway via 192.168.8.1 just: sudo ./set_routeadd via default 192.168.8.1
 Destination and device is not required in default case because Kernel can figure this out.
 All logic is quite simple and sits inside do_route function, you can discover behaviour of this function in different cases above.

 By the way, in few days I'm planning to write new article about this code with more detailed explanations, stay tuned.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/rtnetlink.h>

/* Open netlink socket */
int open_netlink()
{
	struct sockaddr_nl saddr;

	int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

	if (sock < 0) {
		perror("Failed to open netlink socket");
		return -1;
	}

	memset(&saddr, 0, sizeof(saddr));

	saddr.nl_family = AF_NETLINK;
	saddr.nl_pid = getpid();

	if (bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		perror("Failed to bind to netlink socket");
		close(sock);
		return -1;
	}

	return sock;
}

/* Helper structure for ip address data and attributes */

typedef struct {
	char family;
	char bitlen;
	unsigned char data[sizeof(struct in6_addr)];
} _inet_addr;

/* */

#define NLMSG_TAIL(nmsg) \
	((struct rtattr *) (((void *) (nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

/* Add new data to rtattr */
int rtattr_add(struct nlmsghdr *n, int maxlen, int type, const void *data, int alen)
{
	int len = RTA_LENGTH(alen);
	struct rtattr *rta;

	if (NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len) > maxlen) {
		fprintf(stderr, "rtattr_add error: message exceeded bound of %d\n", maxlen);
		return -1;
    }

	rta = NLMSG_TAIL(n);
	rta->rta_type = type;
	rta->rta_len = len; 

	if (alen) {
		memcpy(RTA_DATA(rta), data, alen);
	}

	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len);

    return 0;
}

int do_route(int sock, int cmd, int flags, _inet_addr *dst, _inet_addr *gw, int def_gw, int if_idx)
{
	struct {
		struct nlmsghdr n;
		struct rtmsg r;
		char buf[4096];
	} nl_request;

	/* Initialize request structure */
	nl_request.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
	nl_request.n.nlmsg_flags = NLM_F_REQUEST | flags;
	nl_request.n.nlmsg_type = cmd;
	nl_request.r.rtm_family = dst->family;
	nl_request.r.rtm_table = RT_TABLE_MAIN;
	nl_request.r.rtm_scope = RT_SCOPE_NOWHERE;

	/* Set additional flags if NOT deleting route */
	if (cmd != RTM_DELROUTE) {
		nl_request.r.rtm_protocol = RTPROT_BOOT;
		nl_request.r.rtm_type = RTN_UNICAST;
	}

	nl_request.r.rtm_family = dst->family;
	nl_request.r.rtm_dst_len = dst->bitlen;

	/* Select scope, for simplicity we supports here only IPv6 and IPv4 */
	if (nl_request.r.rtm_family == AF_INET6) {
		nl_request.r.rtm_scope = RT_SCOPE_UNIVERSE;
	} else {
		nl_request.r.rtm_scope = RT_SCOPE_LINK;
	}

	/* Set gateway */
	if (gw->bitlen != 0) {
		rtattr_add(&nl_request.n, sizeof(nl_request), RTA_GATEWAY, &gw->data, gw->bitlen / 8);
		nl_request.r.rtm_scope = 0;
		nl_request.r.rtm_family = gw->family;
	}

	/* Don't set destination and interface in case of default gateways */
	if (!def_gw) {
		/* Set destination network */
		rtattr_add(&nl_request.n, sizeof(nl_request), /*RTA_NEWDST*/ RTA_DST, &dst->data, dst->bitlen / 8);

		/* Set interface */
		rtattr_add(&nl_request.n, sizeof(nl_request), RTA_OIF, &if_idx, sizeof(int));
	}

	/* Send message to the netlink */
	return send(sock, &nl_request, sizeof(nl_request), 0);
}

/* Simple parser of the string IP address
 * TODO IPv6
 */
int read_addr(char *addr, _inet_addr *res)
{
	if (strchr(addr, ':')) {
		res->family = AF_INET6;
		res->bitlen = 128;
	} else {
		res->family = AF_INET;
		res->bitlen = 32;
	}

	return inet_pton(res->family, addr, res->data);
}

#define NEXT_CMD_ARG() do { argv++; if (--argc <= 0) exit(-1); } while(0)

int main(int argc, char **argv)
{
	int default_gw = 0;
	int if_idx = 0;
	int nl_sock;
	_inet_addr to_addr = { 0 };
	_inet_addr gw_addr = { 0 };

	int nl_cmd;
	int nl_flags;

	/* Parse command line arguments */
	while (argc > 0) {
		if (strcmp(*argv, "add") == 0) {
			nl_cmd = RTM_NEWROUTE;
			nl_flags = NLM_F_CREATE | NLM_F_EXCL;

		} else if (strcmp(*argv, "del") == 0) {
			nl_cmd = RTM_DELROUTE;
			nl_flags = 0;

		} else if (strcmp(*argv, "to") == 0) {
			NEXT_CMD_ARG(); /* skip "to" and jump to the actual destination addr */

			if (read_addr(*argv, &to_addr) != 1) {
				fprintf(stderr, "Failed to parse destination network %s\n", *argv);
				exit(-1);
			}

		} else if (strcmp(*argv, "dev") == 0) {
			NEXT_CMD_ARG(); /* skip "dev" */

			if_idx = if_nametoindex(*argv);

		} else if (strcmp(*argv, "via") == 0) {
			NEXT_CMD_ARG(); /* skip "via"*/

			/* Instead of gw address user can set here keyword "default" */
			/* Try to read this keyword and jump to the actual gateway addr */
			if (strcmp(*argv, "default") == 0) {
				default_gw = 1;
				NEXT_CMD_ARG();
			}

			if (read_addr(*argv, &gw_addr) != 1) {
				fprintf(stderr, "Failed to parse gateway address %s\n", *argv);
				exit(-1);
			}
		}

		argc--; argv++;
	}

	nl_sock = open_netlink();

	if (nl_sock < 0) {
		exit(-1);
	}

	do_route(nl_sock, nl_cmd, nl_flags, &to_addr, &gw_addr, default_gw, if_idx);

	close (nl_sock);

	return 0;
}

