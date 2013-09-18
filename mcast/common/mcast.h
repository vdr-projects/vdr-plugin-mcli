/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 * modified by Reel Multimedia, http://www.reel-multimedia.com, info@reel-multimedia.com
 * 01042010 DL: use a single thread for reading from network layer (uses less resources)
 *
 */

#ifndef __MCAST_H__
#define __MCAST_H__

typedef void (*client_udp_cb)(unsigned char *buf, int n, void *arg);

typedef struct _UDPContext
{
	struct _UDPContext *next;
	SOCKET udp_fd;
	int ttl;
	int idx;
	int is_multicast;
	int local_port;
	int reuse_socket;
	struct sockaddr_storage dest_addr;
	size_t dest_addr_len;

	client_udp_cb cb;
	void *arg;
	unsigned char *buff;
	int buffmax;
	int bufflen;
	pthread_mutex_t bufflock;
	struct pollfd *pfd;
} UDPContext;

#define	SA	struct sockaddr

#define UDP_TX_BUF_SIZE 131072
#define UDP_RX_BUF_SIZE 131072
#define UDP_PID_BUF_SIZE 1048576
#define MCAST_TTL 16

UDPContext *server_udp_open_host (const char *host, int port, const char *ifname);
UDPContext *server_udp_open (const struct in6_addr *mcg, int port, const char *ifname);
UDPContext *client_udp_open (const struct in6_addr *mcg, int port, const char *ifname);
UDPContext *client_udp_open_host (const char *host, int port, const char *ifname);

int udp_read (UDPContext * s, uint8_t * buf, int size, int timeout, struct sockaddr_storage *from);
int udp_write (UDPContext * s, uint8_t * buf, int size);
int udp_close (UDPContext * s);

#ifndef MULTI_THREAD_RECEIVER
UDPContext *client_udp_open_host_buff (const char *host, int port, const char *ifname, int buff_size);
UDPContext *client_udp_open_cb   (const struct in6_addr *mcg, int port, const char *ifname, client_udp_cb cb, void *arg);
UDPContext *client_udp_open_buff (const struct in6_addr *mcg, int port, const char *ifname, int buff_size);
int udp_read_buff (UDPContext * s, uint8_t * buf, int size, int timeout, struct sockaddr_storage *from);
int udp_close_buff (UDPContext * s);
#endif

int udp_ipv6_join_multicast_group (SOCKET sockfd, int iface, struct sockaddr *addr);
int udp_ipv6_leave_multicast_group (SOCKET sockfd, int iface, struct sockaddr *addr);
#endif
