/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#ifndef __INTERFACES_H
#define __INTERFACES_H

#define INTNODE_MAXIPV4 4	/* Maximum number of IPv4 aliases */
#define bool 	int

/*
 * The list of interfaces we do multicast on
 * These are discovered on the fly, very handy ;)
 */
struct intnode
{
	unsigned int ifindex;	/* The ifindex */
	char name[IFNAMSIZ];	/* Name of the interface */
	unsigned int groupcount;	/* Number of groups this interface joined */
	unsigned int mtu;	/* The MTU of this interface (mtu = 0 -> invalid interface) */

	struct sockaddr hwaddr;	/* Hardware bytes */

	struct in6_addr linklocal;	/* Link local address */
	struct in6_addr global;	/* Global unicast address */

	/* Per interface statistics */
	uint32_t stat_packets_received;	/* Number of packets received */
	uint32_t stat_packets_sent;	/* Number of packets sent */
	uint32_t stat_bytes_received;	/* Number of bytes received */
	uint32_t stat_bytes_sent;	/* Number of bytes sent */
	uint32_t stat_icmp_received;	/* Number of ICMP's received */
	uint32_t stat_icmp_sent;	/* Number of ICMP's sent */
};

/* Node functions */
struct intnode *int_create (unsigned int ifindex);
void int_destroy (struct intnode *intn);
void update_interfaces (struct intnode *intn);

/* List functions */
struct intnode *int_find (unsigned int ifindex);
struct intnode *int_find_name (char *ifname);
struct intnode *int_find_first (void);
#if defined WIN32 || ! defined __CYGWIN__
unsigned if_nametoindex (const char *ifname);
#endif
#endif
