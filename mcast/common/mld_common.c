/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#include "headers.h"
struct conf *g_conf=NULL;

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef DEBUG
struct lookup icmpv6_types[] = {
	{ICMP6_DST_UNREACH, "Destination Unreachable"},
	{ICMP6_PACKET_TOO_BIG, "Packet too big"},
	{ICMP6_TIME_EXCEEDED, "Time Exceeded"},
	{ICMP6_PARAM_PROB, "Parameter Problem"},
	{ICMP6_ECHO_REQUEST, "Echo Request"},
	{ICMP6_ECHO_REPLY, "Echo Reply"},
	{ICMP6_MEMBERSHIP_QUERY, "Membership Query"},
	{ICMP6_MEMBERSHIP_REPORT, "Membership Report"},
	{ICMP6_MEMBERSHIP_REDUCTION, "Membership Reduction"},
	{ICMP6_V2_MEMBERSHIP_REPORT, "Membership Report (V2)"},
	{ICMP6_V2_MEMBERSHIP_REPORT_EXP, "Membership Report (V2) - Experimental"},
	{ND_ROUTER_SOLICIT, "ND Router Solicitation"},
	{ND_ROUTER_ADVERT, "ND Router Advertisement"},
	{ND_NEIGHBOR_SOLICIT, "ND Neighbour Solicitation"},
	{ND_NEIGHBOR_ADVERT, "ND Neighbour Advertisement"},
	{ND_REDIRECT, "ND Redirect"},
	{ICMP6_ROUTER_RENUMBERING, "Router Renumbering",},
	{ICMP6_NI_QUERY, "Node Information Query"},
	{ICMP6_NI_REPLY, "Node Information Reply"},
	{MLD_MTRACE_RESP, "Mtrace Response"},
	{MLD_MTRACE, "Mtrace Message"},
	{0, NULL},
}, icmpv6_codes_unreach[] = {
	{ICMP6_DST_UNREACH_NOROUTE, "No route to destination"}, 
	{ICMP6_DST_UNREACH_ADMIN, "Administratively prohibited"}, 
	{ICMP6_DST_UNREACH_NOTNEIGHBOR, "Not a neighbor (obsolete)"}, 
	{ICMP6_DST_UNREACH_BEYONDSCOPE, "Beyond scope of source address"}, 
	{ICMP6_DST_UNREACH_ADDR, "Address Unreachable"}, 
	{ICMP6_DST_UNREACH_NOPORT, "Port Unreachable"},
}, icmpv6_codes_ttl[] = {

	{ICMP6_TIME_EXCEED_TRANSIT, "Time Exceeded during Transit",}, 
	{ICMP6_TIME_EXCEED_REASSEMBLY, "Time Exceeded during Reassembly"},
}, icmpv6_codes_param[] = {

	{ICMP6_PARAMPROB_HEADER, "Erroneous Header Field"}, 
	{ICMP6_PARAMPROB_NEXTHEADER, "Unrecognized Next Header"}, 
	{ICMP6_PARAMPROB_OPTION, "Unrecognized Option"},
}, icmpv6_codes_ni[] = {

	{ICMP6_NI_SUCCESS, "Node Information Successful Reply"}, 
	{ICMP6_NI_REFUSED, "Node Information Request Is Refused"}, 
	{ICMP6_NI_UNKNOWN, "Unknown Qtype"},
}, icmpv6_codes_renumber[] = {

	{ICMP6_ROUTER_RENUMBERING_COMMAND, "Router Renumbering Command"}, 
	{ICMP6_ROUTER_RENUMBERING_RESULT, "Router Renumbering Result"}, 
	{ICMP6_ROUTER_RENUMBERING_SEQNUM_RESET, "Router Renumbering Sequence Number Reset"},
}, mld2_grec_types[] = {

	{MLD2_MODE_IS_INCLUDE, "MLDv2 Mode Is Include"}, 
	{MLD2_MODE_IS_EXCLUDE, "MLDv2 Mode Is Exclude"}, 
	{MLD2_CHANGE_TO_INCLUDE, "MLDv2 Change to Include"}, 
	{MLD2_CHANGE_TO_EXCLUDE, "MLDv2 Change to Exclude"}, 
	{MLD2_ALLOW_NEW_SOURCES, "MLDv2 Allow New Source"}, 
	{MLD2_BLOCK_OLD_SOURCES, "MLDv2 Block Old Sources"},
};
#endif

//----------------------------------------------------------------------------------------------------------------
const char *lookup (struct lookup *l, unsigned int num)
{
	unsigned int i;
	for (i = 0; l && l[i].desc; i++) {
		if (l[i].num != num)
			continue;
		return l[i].desc;
	}
	return "Unknown";
}

const char *icmpv6_type (unsigned int type)
{
#ifdef DEBUG
	return lookup (icmpv6_types, type);
#else
	return "";
#endif
}

const char *icmpv6_code (unsigned int type, unsigned int code)
{
#ifdef DEBUG
	struct lookup *l = NULL;
	switch (type) {
	case ICMP6_DST_UNREACH:
		l = icmpv6_codes_unreach;
		break;
	case ICMP6_TIME_EXCEEDED:
		l = icmpv6_codes_ttl;
		break;
	case ICMP6_PARAM_PROB:
		l = icmpv6_codes_param;
		break;
	case ICMP6_NI_QUERY:
	case ICMP6_NI_REPLY:
		l = icmpv6_codes_ni;
		break;
	case ICMP6_ROUTER_RENUMBERING:
		l = icmpv6_codes_renumber;
		break;
	}
	return lookup (l, code);
#else
	return "";
#endif
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint16_t inchksum (const void *data, uint32_t length)
{
	register long sum = 0;
	register const uint16_t *wrd = (const uint16_t *) data;
	register long slen = (long) length;

	while (slen >= 2) {
		sum += *wrd++;
		slen -= 2;
	}

	if (slen > 0)
		sum += *(const uint8_t *) wrd;

	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return (uint16_t) sum;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint16_t ipv6_checksum (const struct ip6_hdr * ip6, uint8_t protocol, const void *data, const uint16_t length)
{
	struct
	{
		uint16_t length;
		uint16_t zero1;
		uint8_t zero2;
		uint8_t next;
	} pseudo;
	register uint32_t chksum = 0;

	pseudo.length = htons (length);
	pseudo.zero1 = 0;
	pseudo.zero2 = 0;
	pseudo.next = protocol;

	/* IPv6 Source + Dest */
	chksum = inchksum (&ip6->ip6_src, sizeof (ip6->ip6_src) + sizeof (ip6->ip6_dst));
	chksum += inchksum (&pseudo, sizeof (pseudo));
	chksum += inchksum (data, length);

	/* Wrap in the carries to reduce chksum to 16 bits. */
	chksum = (chksum >> 16) + (chksum & 0xffff);
	chksum += (chksum >> 16);

	/* Take ones-complement and replace 0 with 0xFFFF. */
	chksum = (uint16_t) ~ chksum;
	if (chksum == 0UL)
		chksum = 0xffffUL;
	return (uint16_t) chksum;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void sendpacket6 (struct intnode *intn, const struct ip6_hdr *iph, const uint16_t len)
{
	int sent;
#if ! (defined WIN32 || defined APPLE)
	struct sockaddr_ll sa;

	memset (&sa, 0, sizeof (sa));

	sa.sll_family = AF_PACKET;
	sa.sll_protocol = htons (ETH_P_IPV6);
	sa.sll_ifindex = intn->ifindex;
	sa.sll_hatype = intn->hwaddr.sa_family;
	sa.sll_pkttype = 0;
	sa.sll_halen = 6;

	/*
	 * Construct a Ethernet MAC address from the IPv6 destination multicast address.
	 * Per RFC2464
	 */
	sa.sll_addr[0] = 0x33;
	sa.sll_addr[1] = 0x33;
	sa.sll_addr[2] = iph->ip6_dst.s6_addr[12];
	sa.sll_addr[3] = iph->ip6_dst.s6_addr[13];
	sa.sll_addr[4] = iph->ip6_dst.s6_addr[14];
	sa.sll_addr[5] = iph->ip6_dst.s6_addr[15];

	/* Send the packet */
	errno = 0;

#else
//	info("Send on Interface %s@%d len:%d\n",intn->name, intn->ifindex, len);
	struct sockaddr_in6 sa;
	memset (&sa, 0, sizeof (sa));
	
	sa.sin6_family = AF_INET6;
	sa.sin6_addr = iph->ip6_dst;
#endif
#ifdef APPLE
	unsigned char *x=(unsigned char *)iph;
	sent = sendto (g_conf->rawsocket, (_SOTYPE)x+40, len-40, 0, (struct sockaddr *) &sa, sizeof (sa));
#else
	sent = sendto (g_conf->rawsocket, (_SOTYPE)iph, len, 0, (struct sockaddr *) &sa, sizeof (sa));
#endif	
	if (sent < 0) {
		/*
		 * Remove the device if it doesn't exist anymore,
		 * can happen with dynamic tunnels etc
		 */
		if (errno == ENXIO) {
			warn ("Cannot send %u bytes on interface %s received ENXIO, interface %u no longer usable\n", len, intn->name, intn->ifindex);
			/* Destroy the interface itself */
			int_destroy (intn);
		} else
			warn ("Cannot send %u bytes on interface %s (%d) failed with a mtu of %u: %s (errno %d)\n", len, intn->name, intn->ifindex, intn->mtu, strerror (errno), errno);
		return;
	}

	/* Update the global statistics */
	g_conf->stat_packets_sent++;
	g_conf->stat_bytes_sent += len;

	/* Update interface statistics */
	intn->stat_bytes_sent += len;
	intn->stat_packets_sent++;
	return;
}
