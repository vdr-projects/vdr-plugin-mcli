/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#include "headers.h"

void int_destroy (struct intnode *intn)
{
	dbg ("Destroying interface %s\n", intn->name);

	/* Resetting the MTU to zero disabled the interface */
	intn->mtu = 0;
}

struct intnode *int_find (unsigned int ifindex)
{
	unsigned int i;	
	for (i = 0; i < g_conf->maxinterfaces; i++) {
		if(g_conf->ints[i].ifindex == ifindex) {
			return g_conf->ints+i;
		}
	}
	return NULL;
}

struct intnode *int_find_name (char *ifname)
{
	unsigned int i;
	for (i = 0; i < g_conf->maxinterfaces; i++) {
		if (!strcmp (ifname, g_conf->ints[i].name) && g_conf->ints[i].mtu != 0) {
			return g_conf->ints+i;
		}
	}
	return NULL;
}


struct intnode *int_find_first (void)
{
	unsigned int i;
	for (i = 0; i < g_conf->maxinterfaces; i++) {
		dbg("int: %d %s\n",i, g_conf->ints[i].name);
		if (g_conf->ints[i].mtu != 0) {
			return g_conf->ints+i;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#if ! (defined WIN32 || defined APPLE) || defined __CYGWIN__

/* Initiliaze interfaces */
void update_interfaces (struct intnode *intn)
{
	struct in6_addr addr;

	FILE *file;
	unsigned int prefixlen, scope, flags, ifindex;
	char devname[IFNAMSIZ];

	/* Only update every 5 seconds to avoid rerunning it every packet */
	if (g_conf->maxinterfaces)
		return;

	dbg ("Updating Interfaces\n");

	/* Get link local addresses from /proc/net/if_inet6 */
	file = fopen ("/proc/net/if_inet6", "r");

	/* We can live without it though */
	if (!file) {
		err ("Cannot open /proc/net/if_inet6\n");
		return;
	}

	char buf[255];
	/* Format "fe80000000000000029027fffe24bbab 02 0a 20 80     eth0" */
	while (fgets (buf, sizeof (buf), file)) {
		if (21 != sscanf (buf, "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx %x %x %x %x %8s", &addr.s6_addr[0], &addr.s6_addr[1], &addr.s6_addr[2], &addr.s6_addr[3], &addr.s6_addr[4], &addr.s6_addr[5], &addr.s6_addr[6], &addr.s6_addr[7], &addr.s6_addr[8], &addr.s6_addr[9], &addr.s6_addr[10], &addr.s6_addr[11], &addr.s6_addr[12], &addr.s6_addr[13], &addr.s6_addr[14], &addr.s6_addr[15], &ifindex, &prefixlen, &scope, &flags, devname)) {

			warn ("/proc/net/if_inet6 in wrong format!\n");
			continue;
		}
		if (!IN6_IS_ADDR_LINKLOCAL (&addr) && (IN6_IS_ADDR_UNSPECIFIED (&addr) || IN6_IS_ADDR_LOOPBACK (&addr) || IN6_IS_ADDR_MULTICAST (&addr))) {
			continue;
		}

		if((intn=int_find(ifindex))==NULL) {
			g_conf->ints=(struct intnode*)realloc(g_conf->ints, sizeof(struct intnode)*(++g_conf->maxinterfaces));
			if (!g_conf->ints) {
				err ("Cannot get memory for interface structures.\n");
			}
			intn=g_conf->ints+g_conf->maxinterfaces-1;
			memset(intn, 0, sizeof(struct intnode));
		}
#ifdef WIN32
		// Ugly WINXP workaround
		if(scope==0x20 && flags==0x80) {
			intn->mtu=1480;
		} else {
			intn->mtu=0;
		}
#else
		intn->ifindex = ifindex;
		strcpy(intn->name, devname);

		struct ifreq ifreq;
		int sock;
		sock = socket (AF_INET6, SOCK_DGRAM, 0);
		if (sock < 0) {
			err ("Cannot get socket for setup\n");
		}

		memcpy (&ifreq.ifr_name, &intn->name, sizeof (ifreq.ifr_name));
		/* Get the MTU size of this interface */
		/* We will use that for fragmentation */
		if (ioctl (sock, SIOCGIFMTU, &ifreq) != 0) {
			warn ("Cannot get MTU size for %s index %d: %s\n", intn->name, intn->ifindex, strerror (errno));
		}
		intn->mtu = ifreq.ifr_mtu;

		/* Get hardware address + type */
		if (ioctl (sock, SIOCGIFHWADDR, &ifreq) != 0) {
			warn ("Cannot get hardware address for %s, interface index %d : %s\n", intn->name, intn->ifindex, strerror (errno));
		}
		intn->hwaddr = ifreq.ifr_hwaddr;
		close (sock);
#endif
		/* Link Local IPv6 address ? */
		if (IN6_IS_ADDR_LINKLOCAL (&addr)) {
			/* Update the linklocal address */
			intn->linklocal = addr;
		} else {
			intn->global = addr;
		}

		dbg ("Available interface %s index %u hardware %s/%u MTU %d\n", intn->name, intn->ifindex, (intn->hwaddr.sa_family == ARPHRD_ETHER ? "Ethernet" : (intn->hwaddr.sa_family == ARPHRD_SIT ? "sit" : "Unknown")), intn->hwaddr.sa_family, intn->mtu);
	}

	fclose (file);
}
#endif 
#if defined WIN32 && ! defined __CYGWIN__

unsigned int if_nametoindex (const char *ifname)
{
	unsigned int ifindex;
	for (ifindex = 0; ifindex < g_conf->maxinterfaces; ifindex++) {
		if (!strcmp (ifname, g_conf->ints[ifindex].name) && g_conf->ints[ifindex].mtu != 0) {
			return g_conf->ints[ifindex].ifindex;
		}
	}
	return 0;
}

void update_interfaces (struct intnode *intn)
{

	/* Declare and initialize variables */

	DWORD dwRetVal = 0;

	int i = 0;

	// Set the flags to pass to GetAdaptersAddresses
	ULONG flags = GAA_FLAG_INCLUDE_PREFIX;

	// default to unspecified address family (both)
	ULONG family = AF_INET6;

	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	ULONG outBufLen = 0;

	PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
	PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;

	outBufLen = sizeof (IP_ADAPTER_ADDRESSES);
	pAddresses = (IP_ADAPTER_ADDRESSES *) malloc (outBufLen);
	if (pAddresses == NULL) {
		printf ("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
		exit (1);
	}
	// Make an initial call to GetAdaptersAddresses to get the 
	// size needed into the outBufLen variable
	if (GetAdaptersAddresses (family, flags, NULL, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW) {
		free (pAddresses);
		pAddresses = (IP_ADAPTER_ADDRESSES *) malloc (outBufLen);
	}

	if (pAddresses == NULL) {
		printf ("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
		exit (1);
	}

	dwRetVal = GetAdaptersAddresses (family, flags, NULL, pAddresses, &outBufLen);

	if (dwRetVal == NO_ERROR) {
		// If successful, output some information from the data we received
		pCurrAddresses = pAddresses;
		g_conf->maxinterfaces=0;
		
		while (pCurrAddresses) {
			
			if( /* pCurrAddresses->Flags & IP_ADAPTER_IPV6_ENABLED && */ (pCurrAddresses->IfType == IF_TYPE_ETHERNET_CSMACD || pCurrAddresses->IfType == IF_TYPE_IEEE80211) && pCurrAddresses->OperStatus == IfOperStatusUp ) {
				g_conf->ints=(struct intnode*)realloc(g_conf->ints, sizeof(struct intnode)*(g_conf->maxinterfaces+1));
				if (!g_conf->ints) {
					err ("update_interfaces: out of memory\n");
				}
				intn=g_conf->ints+g_conf->maxinterfaces;
				memset(intn, 0, sizeof(struct intnode));
				
#ifndef __MINGW32__
				printf ("Interface: %s (%wS)\n", pCurrAddresses->AdapterName, pCurrAddresses->Description);
				dbg ("\tFriendly name: %wS\n", pCurrAddresses->FriendlyName);
#else
				printf ("Interface: %s (%ls)\n", pCurrAddresses->AdapterName, pCurrAddresses->Description);
				dbg ("\tFriendly name: %ls\n", pCurrAddresses->FriendlyName);
#endif
				dbg ("\tFlags: %x\n", pCurrAddresses->Flags);
				dbg ("\tIfType: %ld\n", pCurrAddresses->IfType);
				dbg ("\tOperStatus: %ld\n", pCurrAddresses->OperStatus);
				dbg ("\tMtu: %lu\n", pCurrAddresses->Mtu);
				dbg ("\tIpv6IfIndex (IPv6 interface): %u\n", pCurrAddresses->Ipv6IfIndex);
				
				strncpy(intn->name, pCurrAddresses->AdapterName, IFNAMSIZ-1);
			
				intn->mtu =  pCurrAddresses->Mtu;
				intn->ifindex= pCurrAddresses->Ipv6IfIndex;

				pUnicast = pCurrAddresses->FirstUnicastAddress;
				if (pUnicast != NULL) {
					for (i = 0; pUnicast != NULL; i++) {
						char host[80];
						inet_ntop (AF_INET6, ((struct sockaddr_in6 *)(pUnicast->Address.lpSockaddr))->sin6_addr.s6_addr, host, sizeof(host));
						dbg("\tIP:%s LL:%d\n",host, IN6_IS_ADDR_LINKLOCAL(&((struct sockaddr_in6 *)(pUnicast->Address.lpSockaddr))->sin6_addr));
						if(IN6_IS_ADDR_LINKLOCAL(&((struct sockaddr_in6 *)(pUnicast->Address.lpSockaddr))->sin6_addr)) {
							intn->linklocal=((struct sockaddr_in6 *)(pUnicast->Address.lpSockaddr))->sin6_addr;
						} else {
							intn->global=((struct sockaddr_in6 *)(pUnicast->Address.lpSockaddr))->sin6_addr;
						}
						pUnicast = pUnicast->Next;
					}
					dbg ("\tNumber of Unicast Addresses: %d\n", i);
				} 
#ifdef DEBUG
				if (pCurrAddresses->PhysicalAddressLength != 0) {
					dbg ("\tPhysical address: ");
					for (i = 0; i < (int) pCurrAddresses->PhysicalAddressLength; i++) {
						if (i == (pCurrAddresses->PhysicalAddressLength - 1))
							printf ("%.2X\n", (int) pCurrAddresses->PhysicalAddress[i]);
						else
							printf ("%.2X:", (int) pCurrAddresses->PhysicalAddress[i]);
					}
				}
#endif				
				g_conf->maxinterfaces++;
			}
			pCurrAddresses = pCurrAddresses->Next;
		}
	} 
	
	free (pAddresses);
}

#endif
#ifdef APPLE
void update_interfaces (struct intnode *intn)
{
	struct ifaddrs *myaddrs, *ifa;
	struct sockaddr_in *s4;
	struct sockaddr_in6 *s6;
	int if_index;
	/*
	 * buf must be big enough for an IPv6 address (e.g.
	 * 3ffe:2fa0:1010:ca22:020a:95ff:fe8a:1cf8)
	 */
	char buf[64];

	if (getifaddrs(&myaddrs)) {
		err ("getifaddrs");
	}
	
	for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;
		if ((ifa->ifa_flags & IFF_UP) == 0)
			continue;

		if_index=if_nametoindex(ifa->ifa_name);
		dbg("%s(%d): ", ifa->ifa_name,if_index);
		
		if(!if_index) {
			warn("cannot get interface index for %s\n",ifa->ifa_name);
			continue;
		}

		if((intn=int_find(if_index))==NULL) {
			g_conf->ints=(struct intnode*)realloc(g_conf->ints, sizeof(struct intnode)*(++g_conf->maxinterfaces));
			if (!g_conf->ints) {
				err ("Cannot get memory for interface structures.\n");
			}
			intn=g_conf->ints+g_conf->maxinterfaces-1;
			memset(intn, 0, sizeof(struct intnode));
		}

		intn->ifindex=if_index;
		strcpy(intn->name,ifa->ifa_name);

		if(ifa->ifa_addr->sa_family == AF_LINK && ((struct if_data *)ifa->ifa_data)->ifi_type != IFT_LOOP && ifa->ifa_data) {
			dbg("MTU: %d\n", ((struct if_data *)ifa->ifa_data)->ifi_mtu);
			intn->mtu=((struct if_data *)ifa->ifa_data)->ifi_mtu;
			memcpy(&intn->hwaddr, ifa->ifa_addr, sizeof(struct sockaddr_in));
		}

		if (ifa->ifa_addr->sa_family == AF_INET) {
			s4 = (struct sockaddr_in *) (ifa->ifa_addr);
			if (inet_ntop(ifa->ifa_addr->sa_family, (void *) &(s4->sin_addr), buf, sizeof(buf)) == NULL) {
				warn("%s: inet_ntop failed!\n", ifa->ifa_name);
			} else {
				dbg("%s\n", buf);
			}
		} else if (ifa->ifa_addr->sa_family == AF_INET6) {
			s6 = (struct sockaddr_in6 *) (ifa->ifa_addr);
			/* Link Local IPv6 address ? */
			if (IN6_IS_ADDR_LINKLOCAL (&s6->sin6_addr)) {
				/* Update the linklocal address */
				intn->linklocal = s6->sin6_addr;
			} else {
				intn->global = s6->sin6_addr;
			}
			if (inet_ntop(ifa->ifa_addr->sa_family, (void *) &(s6->sin6_addr), buf, sizeof(buf)) == NULL) {
				warn("%s: inet_ntop failed!\n", ifa->ifa_name);
			} else {
				dbg("%s\n", buf);
			}
		}
	}

	freeifaddrs(myaddrs);
}

#endif
