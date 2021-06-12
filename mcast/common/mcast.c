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

#include "headers.h"
//----------------------------------------------------------------------------------------------------------------------------------
STATIC int udp_ipv6_is_multicast_address (const struct sockaddr *addr)
{
#ifdef IPV4
	if (addr->sa_family == AF_INET)
		return IN_MULTICAST (ntohl (((struct sockaddr_in *) addr)->sin_addr.s_addr));
#endif
	if (addr->sa_family == AF_INET6)
		return IN6_IS_ADDR_MULTICAST (&((struct sockaddr_in6 *) addr)->sin6_addr);
	return -1;
}

//---------------------------------------------------------------------------------------------------------------------------------
STATIC int udp_ipv6_set_multicast_ttl (SOCKET sockfd, int mcastTTL, struct sockaddr *addr)
{
#ifdef IPV4
	if (addr->sa_family == AF_INET) {
		if (setsockopt (sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &mcastTTL, sizeof (mcastTTL)) < 0) {
			perror ("setsockopt(IP_MULTICAST_TTL)");
			return -1;
		}
	}
#endif
	if (addr->sa_family == AF_INET6) {
		if (setsockopt (sockfd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (_SOTYPE)&mcastTTL, sizeof (mcastTTL)) < 0) {
			perror ("setsockopt(IPV6_MULTICAST_HOPS)");
			return -1;
		}
	}
	return 0;
}

//---------------------------------------------------------------------------------------------------------------------------------
int udp_ipv6_join_multicast_group (SOCKET sockfd, int iface, struct sockaddr *addr)
{
#ifdef IPV4
	if (addr->sa_family == AF_INET) {
		struct ip_mreq mreq;
		mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *) addr)->sin_addr.s_addr;
		mreq.imr_interface.s_addr = INADDR_ANY;
		if (setsockopt (sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const void *) &mreq, sizeof (mreq)) < 0) {
			perror ("setsockopt(IP_ADD_MEMBERSHIP)");
			return -1;
		}
	}
#endif
	if (addr->sa_family == AF_INET6) {
		struct ipv6_mreq mreq6;
		memcpy (&mreq6.ipv6mr_multiaddr, &(((struct sockaddr_in6 *) addr)->sin6_addr), sizeof (struct in6_addr));
		mreq6.ipv6mr_interface = iface;
		if (setsockopt (sockfd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (_SOTYPE)&mreq6, sizeof (mreq6)) < 0) {
			perror ("setsockopt(IPV6_ADD_MEMBERSHIP)");
			return -1;
		}
	}
	return 0;
}

//---------------------------------------------------------------------------------------------------------------------------------
int udp_ipv6_leave_multicast_group (SOCKET sockfd, int iface, struct sockaddr *addr)
{
#ifdef IPV4
	if (addr->sa_family == AF_INET) {
		struct ip_mreq mreq;
		mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *) addr)->sin_addr.s_addr;
		mreq.imr_interface.s_addr = INADDR_ANY;
		if (setsockopt (sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const void *) &mreq, sizeof (mreq)) < 0) {
			perror ("setsockopt(IP_DROP_MEMBERSHIP)");
			return -1;
		}
	}
#endif
	if (addr->sa_family == AF_INET6) {
	struct ipv6_mreq mreq6;
		memcpy (&mreq6.ipv6mr_multiaddr, &(((struct sockaddr_in6 *) addr)->sin6_addr), sizeof (struct in6_addr));
		mreq6.ipv6mr_interface = iface;
		if (setsockopt (sockfd, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, (_SOTYPE)&mreq6, sizeof (mreq6)) < 0) {
			perror ("setsockopt(IPV6_DROP_MEMBERSHIP)");
			return -1;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------------------------------------------------------------
STATIC int sockfd_to_family (SOCKET sockfd)
{
	struct sockaddr_storage ss;
	socklen_t len;

	len = sizeof (ss);
	if (getsockname (sockfd, (SA *) & ss, &len) < 0)
		return (-1);
	return (ss.ss_family);
}

/* end sockfd_to_family */
//----------------------------------------------------------------------------------------------------------------------------------
int mcast_set_if (SOCKET sockfd, const char *ifname, u_int ifindex)
{
	switch (sockfd_to_family (sockfd)) {
#ifdef IPV4
	case AF_INET:{
			struct in_addr inaddr;
			struct ifreq ifreq;

			if (ifindex > 0) {
				if (if_indextoname (ifindex, ifreq.ifr_name) == NULL) {
					errno = ENXIO;	/* i/f index not found */
					return (-1);
				}
				goto doioctl;
			} else if (ifname != NULL) {
				memset(&ifreq, 0, sizeof(struct ifreq));
				strncpy (ifreq.ifr_name, ifname, IFNAMSIZ-1);
			      doioctl:
				if (ioctl (sockfd, SIOCGIFADDR, &ifreq) < 0)
					return (-1);
				memcpy (&inaddr, &((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr, sizeof (struct in_addr));
			} else
				inaddr.s_addr = htonl (INADDR_ANY);	/* remove prev. set default */

			return (setsockopt (sockfd, IPPROTO_IP, IP_MULTICAST_IF, &inaddr, sizeof (struct in_addr)));
		}
#endif
	case AF_INET6:{
			u_int idx;
//              printf("Changing interface IPV6...\n");
			if ((idx = ifindex) == 0) {
				if (ifname == NULL) {
					errno = EINVAL;	/* must supply either index or name */
					return (-1);
				}
				if ((idx = if_nametoindex (ifname)) == 0) {
					errno = ENXIO;	/* i/f name not found */
					return (-1);
				}
			}
			return (setsockopt (sockfd, IPPROTO_IPV6, IPV6_MULTICAST_IF, (_SOTYPE)&idx, sizeof (idx)));
		}

	default:
//		errno = EAFNOSUPPORT;
		return (-1);
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------
UDPContext *server_udp_open (const struct in6_addr *mcg, int port, const char *ifname)
{
	UDPContext *s;
	int sendfd;
	int n;

	pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL);

	s = (UDPContext *) calloc (1, sizeof (UDPContext));
	if (!s) {
		err ("Cannot allocate memory !\n");
		goto error;
	}
	struct sockaddr_in6 *addr = (struct sockaddr_in6 *) &s->dest_addr;

	addr->sin6_addr=*mcg;;
	addr->sin6_family = AF_INET6;
	addr->sin6_port = htons (port);
	s->dest_addr_len = sizeof (struct sockaddr_in6);

	sendfd = socket (PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sendfd < 0) {
		err ("cannot get socket\n");
	}

	s->dest_addr_len = sizeof (struct sockaddr_in6);

	if ((udp_ipv6_is_multicast_address ((struct sockaddr *) &s->dest_addr))) {
		if (ifname && strlen (ifname) && (mcast_set_if (sendfd, ifname, 0) < 0)) {
			warn ("mcast_set_if error\n");
			goto error;
		}
		if (udp_ipv6_set_multicast_ttl (sendfd, MCAST_TTL, (struct sockaddr *) &s->dest_addr) < 0) {
			warn ("udp_ipv6_set_multicast_ttl");
		}
	}
	
	n = UDP_TX_BUF_SIZE;
	if (setsockopt (sendfd, SOL_SOCKET, SO_SNDBUF, (_SOTYPE)&n, sizeof (n)) < 0) {
		warn ("setsockopt sndbuf");
	}
	s->is_multicast = 0;	//server
	s->udp_fd = sendfd;
	s->local_port = port;

	dbg ("Multicast streamer initialized successfully ! \n");

	pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
	return s;
      error:
	err ("Cannot init udp_server  !\n");
	if (s) {
		free (s);
	}

	pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
	return NULL;
}

UDPContext *server_udp_open_host (const char *host, int port, const char *ifname)
{
	struct in6_addr addr;

	inet_pton (AF_INET6, host, &addr);

	return server_udp_open (&addr, port, ifname);
}

//--------------------------------------------------------------------------------------------------------------------------------------------
UDPContext *client_udp_open (const struct in6_addr *mcg, int port, const char *ifname)
{
	UDPContext *s;
	int recvfd;
	int n;

	pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL);

	s = (UDPContext *) calloc (1, sizeof (UDPContext));
	if (!s) {
		err ("%s: cannot allocate memory\n", __FUNCTION__);
		goto error;
	}

	struct sockaddr_in6 *addr = (struct sockaddr_in6 *) &s->dest_addr;
#ifndef WIN32
	addr->sin6_addr=*mcg;
#else
	struct in6_addr any;
	memset(&any,0,sizeof(any));
	addr->sin6_addr=any;
#endif
	addr->sin6_family = AF_INET6;
	addr->sin6_port = htons (port);
	s->dest_addr_len = sizeof (struct sockaddr_in6);

	recvfd = socket (PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (recvfd < 0) {
		err ("%s: cannot get socket\n", __FUNCTION__);
	}
#ifdef WIN32	
# ifndef IPV6_PROTECTION_LEVEL
#   define IPV6_PROTECTION_LEVEL 23
#  endif
        n = 10 /*PROTECTION_LEVEL_UNRESTRICTED*/;
        if(setsockopt( recvfd, IPPROTO_IPV6, IPV6_PROTECTION_LEVEL, (_SOTYPE)&n, sizeof(n) ) < 0 ) {
        	warn ("%s: setsockopt IPV6_PROTECTION_LEVEL\n", __FUNCTION__);
        }
#endif                                    
	n = 1;
	if (setsockopt (recvfd, SOL_SOCKET, SO_REUSEADDR, (_SOTYPE)&n, sizeof (n)) < 0) {
		warn ("%s: setsockopt REUSEADDR\n", __FUNCTION__);
	}

#if ! (defined WIN32 || defined APPLE)
	if (ifname && strlen (ifname) && setsockopt (recvfd, SOL_SOCKET, SO_BINDTODEVICE, ifname, strlen (ifname) + 1)) {
		dbg ("%s: setsockopt SO_BINDTODEVICE %s failed\n", __FUNCTION__, ifname);
	}
#endif
	if (bind (recvfd, (struct sockaddr *) &s->dest_addr, s->dest_addr_len) < 0) {
		warn ("%s: bind failed on interface=%s for port=%d\n", __FUNCTION__, ifname, port);
		goto error;
	}
#ifdef WIN32
	addr->sin6_addr=*mcg;
#endif
	if (udp_ipv6_is_multicast_address ((struct sockaddr *) &s->dest_addr)) {
#if 0
		if (ifname && strlen (ifname) && (mcast_set_if (recvfd, ifname, 0) < 0)) {
			warn ("mcast_set_if error \n");
			goto error;
		}
#endif
		if (ifname) {
			if ((s->idx = if_nametoindex (ifname)) == 0) {
				s->idx = 0;
			} else {
				dbg("Selecting interface %s (%d)", ifname, s->idx);
			}
		} else {
			s->idx = 0;
		}

		if (udp_ipv6_join_multicast_group (recvfd, s->idx, (struct sockaddr *) &s->dest_addr) < 0) {
			warn ("%s: cannot join multicast group\n", __FUNCTION__);
			goto error;
		}
		s->is_multicast = 1;
	}

	n = UDP_RX_BUF_SIZE;
	if (setsockopt (recvfd, SOL_SOCKET, SO_RCVBUF, (_SOTYPE)&n, sizeof (n)) < 0) {
		warn ("%s: setsockopt rcvbuf", __FUNCTION__);
		goto error;
	}

	s->udp_fd = recvfd;
	s->local_port = port;

	pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);

	return s;
      error:
	warn ("%s: error on interface=%s for port=%d\n", __FUNCTION__, ifname, port);
	if (s) {
		free (s);
	}
	pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
	return NULL;
}

UDPContext *client_udp_open_host (const char *host, int port, const char *ifname)
{
	struct in6_addr addr;

	inet_pton (AF_INET6, host, &addr);

	return client_udp_open (&addr, port, ifname);
}

//--------------------------------------------------------------------------------------------------------------------------------------------
int udp_read (UDPContext * s, uint8_t * buf, int size, int timeout, struct sockaddr_storage *from)
{
	socklen_t from_len = sizeof (struct sockaddr_storage);
	struct sockaddr_storage from_local;
	
	if(!from) {
		from=&from_local;
	}
	
	struct pollfd p;	
	p.fd = s->udp_fd;
	p.events = POLLIN;

	if(poll(&p,1,(timeout+999)>>10)>0) {
		return recvfrom (s->udp_fd, (char *)buf, size, 0, (struct sockaddr *) from, &from_len);
	}
	return -1;
}

//--------------------------------------------------------------------------------------------------------------------------------------------------
int udp_write (UDPContext * s, uint8_t * buf, int size)
{
	int ret;

	for (;;) {
		ret = sendto (s->udp_fd, (char *) buf, size, 0, (struct sockaddr *) &s->dest_addr, s->dest_addr_len);

		if (ret < 0) {
			if (errno != EINTR && errno != EAGAIN)
				return -1;
		} else {
			break;
		}
	}
	return size;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
int udp_close (UDPContext * s)
{
	if (s->is_multicast)
		udp_ipv6_leave_multicast_group (s->udp_fd, s->idx, (struct sockaddr *) &s->dest_addr);

	closesocket (s->udp_fd);
	free (s);

	return 0;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

#ifndef MULTI_THREAD_RECEIVER

#define MAX_BUFF_SIZE 0x10000
#define MAX_CON_LIST 128
UDPContext *gConList[MAX_CON_LIST];
pthread_mutex_t gConListLock = PTHREAD_MUTEX_INITIALIZER;
static int gConListInit=0;
static int gConListModified;
static UDPContext *gConChain;

STATIC void client_upd_cleanup (void *arg) {
	if(!gConListInit) return;
	pthread_mutex_lock(&gConListLock);
  memset(&gConList, 0, sizeof(gConList));
  gConListInit=0;
	pthread_mutex_unlock(&gConListLock);
} // client_upd_cleanup

void *client_upd_process(void *arg) {
#ifdef RT
#if 1
	if (setpriority (PRIO_PROCESS, 0, -15) == -1)
#else
	if (pthread_setschedprio (p->recv_ts_thread, -15))
#endif
	{
		dbg ("Cannot raise priority to -15\n");
	}
#endif
	unsigned char buff[MAX_BUFF_SIZE];
	socklen_t from_len = sizeof (struct sockaddr_storage);
	struct sockaddr_storage from_local;
	struct pollfd fds[MAX_CON_LIST];

	pthread_cleanup_push (client_upd_cleanup, 0);
	int max_fd=0;
	while(1) {
		UDPContext *e;
		pthread_mutex_lock(&gConListLock);

		if(gConListModified) {
			gConListModified=0;
			max_fd=0;
			for(e=gConChain;e;e=e->next) {
				fds[max_fd].fd = e->udp_fd;
				fds[max_fd].events = POLLIN;
				fds[max_fd].revents = 0;
				e->pfd = &fds[max_fd];
				max_fd++;
			} // for
		} // if
		pthread_mutex_unlock(&gConListLock);
		int rs = poll(fds, max_fd, 1000);
		if(rs>0) {
			pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL);
			pthread_mutex_lock(&gConListLock);
			for(e=gConChain;e;e=e->next) {
				if(e->pfd && (e->pfd->revents & POLLIN)) {
					if(e->cb) {
						int ret = recvfrom (e->udp_fd, (char *)buff, MAX_BUFF_SIZE, 0, 0, 0/*(struct sockaddr *) &from_local, &from_len*/);
						if(ret>0)
							e->cb(buff, ret, e->arg);
					} else if(e->buff && !e->bufflen) {
						pthread_mutex_lock(&e->bufflock);
						int ret = recvfrom (e->udp_fd, (char *)e->buff, e->buffmax, 0, (struct sockaddr *) &from_local, &from_len);
						if(ret>0)
							e->bufflen = ret;
						pthread_mutex_unlock(&e->bufflock);
					} // if
				} // if
			} // for
			pthread_mutex_unlock(&gConListLock);
			pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
		} // if
		pthread_testcancel();			
	} // while
	pthread_cleanup_pop (1);
	return NULL;
}

static int client_upd_init() {
	pthread_mutex_lock(&gConListLock);
	if(gConListInit) {
		pthread_mutex_unlock(&gConListLock);
		return 1;
	} // if
	memset(&gConList, 0, sizeof(gConList));
	gConListModified = 0;
	gConChain = NULL;
	pthread_t client_upd_thread;
	if(0==pthread_create (&client_upd_thread, NULL, client_upd_process, 0)) {
		gConListInit = 1;
		pthread_detach(client_upd_thread);
	} // if
	pthread_mutex_unlock(&gConListLock);
	return gConListInit;
} // client_upd_init

UDPContext *client_udp_open_buff (const struct in6_addr *mcg, int port, const char *ifname, int buff_size) {
	UDPContext *ret = client_udp_open_cb (mcg, port, ifname, 0, 0);
	if(ret) {
		ret->buff     = (unsigned char *)malloc(buff_size);
		ret->buffmax  = buff_size;
		ret->bufflen  = 0;
		if (!ret->buff) {
			err ("client_udp_open_buff: out of memory\n");
		}
	} // if
	return ret;
} // client_udp_open_buff

UDPContext *client_udp_open_cb (const struct in6_addr *mcg, int port, const char *ifname, client_udp_cb cb, void *arg)
{
	if(!client_upd_init()) return NULL;

	UDPContext *s;
	int recvfd = -1;
	int n;

	pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL);

	s = (UDPContext *) calloc (1, sizeof (UDPContext));
	if (!s) {
		err ("%s: cannot allocate memory\n",  __FUNCTION__);
		goto error;
	}

	struct sockaddr_in6 *addr = (struct sockaddr_in6 *) &s->dest_addr;
#ifndef WIN32
	addr->sin6_addr=*mcg;
#else
	struct in6_addr any=IN6ADDR_ANY_INIT;
	addr->sin6_addr=any;
#endif
	addr->sin6_family = AF_INET6;
	addr->sin6_port = htons (port);
	s->dest_addr_len = sizeof (struct sockaddr_in6);

	recvfd = socket (PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (recvfd < 0) {
		err ("%s: cannot get socket\n", __FUNCTION__);
	}
#ifdef WIN32	
# ifndef IPV6_PROTECTION_LEVEL
#   define IPV6_PROTECTION_LEVEL 23
#  endif
        n = 10 /*PROTECTION_LEVEL_UNRESTRICTED*/;
        if(setsockopt( recvfd, IPPROTO_IPV6, IPV6_PROTECTION_LEVEL, (_SOTYPE)&n, sizeof(n) ) < 0 ) {
        	warn ("%s: setsockopt IPV6_PROTECTION_LEVEL\n", __FUNCTION__);
        }
#endif                                    
	n = 1;
	if (setsockopt (recvfd, SOL_SOCKET, SO_REUSEADDR, (_SOTYPE)&n, sizeof (n)) < 0) {
		warn ("%s: setsockopt REUSEADDR\n", __FUNCTION__);
	}

#if ! (defined WIN32 || defined APPLE)
	if (ifname && strlen (ifname) && setsockopt (recvfd, SOL_SOCKET, SO_BINDTODEVICE, ifname, strlen (ifname) + 1)) {
		dbg ("%s: setsockopt SO_BINDTODEVICE %s failed\n", __FUNCTION__, ifname);
	}
#endif
	if (bind (recvfd, (struct sockaddr *) &s->dest_addr, s->dest_addr_len) < 0) {
		warn ("%s: bind failed on interface=%s\n", __FUNCTION__, ifname);
		goto error;
	}
#ifdef WIN32
	addr->sin6_addr=*mcg;
#endif
	if (udp_ipv6_is_multicast_address ((struct sockaddr *) &s->dest_addr)) {
#if 0
		if (ifname && strlen (ifname) && (mcast_set_if (recvfd, ifname, 0) < 0)) {
			warn ("mcast_set_if error \n");
			goto error;
		}
#endif
		if (ifname) {
			if ((s->idx = if_nametoindex (ifname)) == 0) {
				s->idx = 0;
			} else {
				dbg("Selecting interface %s (%d)", ifname, s->idx);
			}
		} else {
			s->idx = 0;
		}

		if (udp_ipv6_join_multicast_group (recvfd, s->idx, (struct sockaddr *) &s->dest_addr) < 0) {
			warn ("%s: cannot join multicast group\n", __FUNCTION__);
			goto error;
		}
		s->is_multicast = 1;
	}

	n = cb ? UDP_PID_BUF_SIZE : UDP_RX_BUF_SIZE;
	if (setsockopt (recvfd, SOL_SOCKET, SO_RCVBUF, (_SOTYPE)&n, sizeof (n)) < 0) {
		warn ("%s: setsockopt rcvbuf", __FUNCTION__);
		goto error;
	}

	s->udp_fd = recvfd;
	s->local_port = port;

	pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);

	s->cb       = cb;
	s->arg      = arg;
	pthread_mutex_init(&s->bufflock, NULL);
	int i;
	pthread_mutex_lock(&gConListLock);
	for(i=0;i<MAX_CON_LIST;i++) {
		if(!gConList[i]) {
			gConList[i]=s;
			gConListModified=1;
			s->next=gConChain;
			gConChain=s;
			break;
		} // if
	} // for
	pthread_mutex_unlock(&gConListLock);
	if(i>=MAX_CON_LIST)
		warn("---------------------------------------------No slot found!\n");

	return s;
      error:
	warn ("%s: socket error on interface=%s\n", __FUNCTION__, ifname);
	if (s) {
		free (s);
	}
	pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
	return NULL;
}

UDPContext *client_udp_open_host_buff (const char *host, int port, const char *ifname, int buff_size)
{
	struct in6_addr addr;

	inet_pton (AF_INET6, host, &addr);

	return client_udp_open_buff (&addr, port, ifname, buff_size);
}

//--------------------------------------------------------------------------------------------------------------------------------------------
int udp_read_buff (UDPContext * s, uint8_t * buf, int size, int timeout, struct sockaddr_storage *from)
{
	pthread_mutex_lock(&s->bufflock);
	int ret = s->bufflen>size ? size : s->bufflen;
	if(ret>0) {
		memcpy(buf, s->buff, ret);
		s->bufflen-=ret;
	}
	pthread_mutex_unlock(&s->bufflock);
	return ret;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------
int udp_close_buff (UDPContext * s)
{
	int i;
	pthread_mutex_lock(&gConListLock);
	for(i=0;i<MAX_CON_LIST;i++) {
		if(gConList[i] == s) {
			gConList[i]=0;
			gConListModified=1;
			UDPContext **e;
			for(e=&gConChain;*e;e=&(*e)->next) {
				if(*e == s) {
					*e=(*e)->next;
					break;
				}
			}
			break;
		}
	}
	pthread_mutex_unlock(&gConListLock);
	if (s->is_multicast)
		udp_ipv6_leave_multicast_group (s->udp_fd, s->idx, (struct sockaddr *) &s->dest_addr);

	closesocket (s->udp_fd);
	free(s->buff);
	pthread_mutex_destroy(&s->bufflock);
	free (s);

	return 0;
}
#endif
