/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#undef DEBUG
#include "headers.h"

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int send_mldv2_report (struct intnode *intn, int grec_number, struct in6_addr *mcas, int srcn, struct in6_addr *sources, int mode)
{
	unsigned int length;
	struct mld_report_packet
	{
		struct ip6_hdr ip6;
		struct ip6_hbh hbh;
		struct
		{
			uint8_t type;
			uint8_t length;
			uint16_t value;
			uint8_t optpad[2];
		} routeralert;
		struct mld2_report mld2r;
	} *packet;
	struct mld2_grec *grec = NULL;
	struct in6_addr *src = NULL;
	int mca_index, src_index;
	int count = 0;

	//printf("creating multicast listener report packet....\n");
	//printf("size src = %d size grec = %d \n",sizeof(*src),sizeof(*grec));
	if (intn->mtu < sizeof (*packet)) {
		/*
		 * MTU is too small to support this type of packet
		 * Should not happen though
		 */
		dbg ("MTU too small for packet while sending MLDv2 report on interface %s/%u mtu=%u!?\n", intn->name, intn->ifindex, intn->mtu);
		return -1;
	}

	/* Allocate a buffer matching the MTU size of this interface */
	packet = (struct mld_report_packet *) malloc (intn->mtu);

	if (!packet) {
		err ("Cannot get memory for MLD2 report packet, aborting\n");
	}
//      printf("addr packet  = %u \n",packet);
	memset (packet, 0, intn->mtu);


	/* Create the IPv6 packet */
	packet->ip6.ip6_vfc = 0x60;
	packet->ip6.ip6_plen = ntohs (sizeof (*packet) - sizeof (packet->ip6));
	packet->ip6.ip6_nxt = IPPROTO_HOPOPTS;
	packet->ip6.ip6_hlim = 1;

	/*
	 * The source address must be the link-local address
	 * of the interface we are sending on
	 */
	memcpy (&packet->ip6.ip6_src, &intn->linklocal, sizeof (packet->ip6.ip6_src));

	/* MLDv2 Report -> All IPv6 Multicast Routers (ff02::16) */
	packet->ip6.ip6_dst.s6_addr[0] = 0xff;
	packet->ip6.ip6_dst.s6_addr[1] = 0x02;
	packet->ip6.ip6_dst.s6_addr[15] = 0x16;

	/* HopByHop Header Extension */
	packet->hbh.ip6h_nxt = IPPROTO_ICMPV6;
	packet->hbh.ip6h_len = 0;

	/* Router Alert Option */
	packet->routeralert.type = 5;
	packet->routeralert.length = sizeof (packet->routeralert.value);
	packet->routeralert.value = 0;	/* MLD ;) */

	/* Option Padding */
	packet->routeralert.optpad[0] = IP6OPT_PADN;
	packet->routeralert.optpad[1] = 0;

	/* ICMPv6 MLD Report */
	packet->mld2r.type = ICMP6_V2_MEMBERSHIP_REPORT;
	//number of multi cast address reocrds
	packet->mld2r.ngrec = 0;	//grec_number;//htons(1);
	length = 0;
	count = 0;
	for (mca_index = 0; mca_index < grec_number; mca_index++) {
		src = NULL;
		if (!grec) {
			length = sizeof (*packet) + sizeof (*grec) - sizeof (packet->ip6);
			//fprintf(stdout,"(grec = %02d) %02d. current report length = %04d MTU: %04d)\n",packet->mld2r.ngrec,mca_index,length+sizeof(packet->ip6),intn->mtu);

			if (length + sizeof (packet->ip6) > intn->mtu) {
				/* Should not happen! Would mean the MTU is smaller than a standard mld report */
				dbg ("No grec and MTU too small for packet while sending MLDv2 report on interface %s/%u mtu=%u!?\n", intn->name, intn->ifindex, intn->mtu);
				free (packet);
				return (-1);
			} else
				grec = (struct mld2_grec *) (((char *) packet) + sizeof (*packet));

			packet->mld2r.ngrec++;
		} else {
			if (!src)
				length = ((((char *) grec) - ((char *) packet)) + sizeof (*grec) + (sizeof (*src) * (grec->grec_nsrcs))) - sizeof (packet->ip6);

			//fprintf(stdout,"\nloop1:(grec = %02d) %02d.length = %04d (Total : %04d  MTU: %04d)\n\n", packet->mld2r.ngrec , mca_index,length,(length + sizeof(packet->ip6)), intn->mtu);

			if (((length + sizeof (packet->ip6) + sizeof (*grec)) > intn->mtu)) {

				/* Take care of endianess */
				//fprintf(stdout,"next grec record does not fit... sending... \n");

				packet->mld2r.ngrec = htons (packet->mld2r.ngrec);
				grec->grec_nsrcs = htons (grec->grec_nsrcs);

				/* Calculate and fill in the checksum */
				packet->ip6.ip6_plen = htons (length);
				packet->mld2r.csum = htons (0);
				packet->mld2r.csum = ipv6_checksum (&packet->ip6, IPPROTO_ICMPV6, (uint8_t *) & packet->mld2r, length - sizeof (struct ip6_hbh) - sizeof (packet->routeralert));
				count++;
#ifdef DEBUG
				dbg ("%04d.Sending2 MLDv2 Report on %s/%u, ngrec=%u, length=%u sources=%u (in last grec)\n", count, intn->name, intn->ifindex, ntohs (packet->mld2r.ngrec), length, ntohs (grec->grec_nsrcs));
				sendpacket6 (intn, (const struct ip6_hdr *) packet, length + sizeof (packet->ip6));
#endif
				/* Increase ICMP sent statistics */
				g_conf->stat_icmp_sent++;
				intn->stat_icmp_sent++;

				/* Reset the MLDv2 struct */
				packet->mld2r.ngrec = 0;
				grec = (struct mld2_grec *) (((char *) packet) + sizeof (*packet));
			} else {
				//next grec
				grec->grec_nsrcs = htons (grec->grec_nsrcs);
				grec = (struct mld2_grec *) (((char *) grec) + sizeof (*grec) + (sizeof (*src) * ntohs (grec->grec_nsrcs)));

			}
			packet->mld2r.ngrec++;
		}
		//Copy MCA to record
		memcpy (&grec->grec_mca, mcas + mca_index, sizeof (grec->grec_mca));

		/* Zero sources upto now */
		grec->grec_nsrcs = 0;
		/* 0 Sources -> Exclude those */
		grec->grec_type = MLD2_MODE_IS_EXCLUDE;
		if (mode) {
			grec->grec_type = mode;
		}

		/* Nothing added yet */
		for (src_index = 0; src_index < srcn || (!srcn && src_index == 0); src_index++) {

			//check for duplicate source reocrds and any address

			/* Packet with at least one grec and one or more src's, excluding ip6 header */

			length = (((char *) grec) - ((char *) packet)) + sizeof (*grec) + (sizeof (*src) * (grec->grec_nsrcs)) - sizeof (packet->ip6);

			//fprintf(stdout,"loop2:(grec = %02d) %02d.length = %04d (Total : %04d  MTU: %04d)\n", packet->mld2r.ngrec,mca_index,length,(length + sizeof(packet->ip6)),intn->mtu);
			/* Would adding it not fit? -> Send it out */
			if (((length + sizeof (packet->ip6) + sizeof (*src)) > (intn->mtu)) && srcn) {
				//fprintf(stdout,"next source addr. does not fit... sending... \n");
				//fprintf(stdout,"src_index = %d grec->grec_nsrcs = %d \n",src_index,grec->grec_nsrcs);

				/* Take care of endianess */
				packet->mld2r.ngrec = htons (packet->mld2r.ngrec);
				grec->grec_nsrcs = htons (grec->grec_nsrcs);
				/* Calculate and fill in the checksum */

				packet->ip6.ip6_plen = htons (length);
				packet->mld2r.csum = htons (0);
				packet->mld2r.csum = ipv6_checksum (&packet->ip6, IPPROTO_ICMPV6, (uint8_t *) & packet->mld2r, length - sizeof (struct ip6_hbh) - sizeof (packet->routeralert));

				count++;
				dbg ("%04d.Sending2 MLDv2 Report on %s/%u, ngrec=%u, length=%u sources=%u (in last grec)\n", count, intn->name, intn->ifindex, ntohs (packet->mld2r.ngrec), length, ntohs (grec->grec_nsrcs));
				sendpacket6 (intn, (const struct ip6_hdr *) packet, length + sizeof (packet->ip6));

				/* Increase ICMP sent statistics */
				g_conf->stat_icmp_sent++;
				intn->stat_icmp_sent++;

				/* Reset the MLDv2 struct */
				packet->mld2r.ngrec = 0;
				src = NULL;
				grec = NULL;
				//if not IS_EX or TO EXCLUDE_MODE splitting must be done
				break;
			}

			/* Only add non-:: addresses */
			if (!srcn)
				break;
			if (!IN6_IS_ADDR_UNSPECIFIED (sources + src_index) && srcn) {
				/* First/Next address */
				src = (struct in6_addr *) (((char *) grec) + sizeof (*grec) + (sizeof (*src) * grec->grec_nsrcs));
				/* An additional source */
				grec->grec_nsrcs++;
				if (mode) {
					grec->grec_type = mode;	//MLD2_MODE_IS_EXCLUDE;
				}
				/* Append the source address */
				memcpy (src, sources + src_index, sizeof (*src));
			}
		}
	}

	//fprintf(stdout,"done\n");
	if (packet->mld2r.ngrec == 0) {
		//fprintf(stdout,"All data sent !!!!!!\n");
		free (packet);
		packet = NULL;
		return (1);
	}
	/* Take care of endianess */
	length = (((char *) grec) - ((char *) packet) + sizeof (*grec) + (sizeof (*src) * (grec->grec_nsrcs)) - sizeof (packet->ip6));
	packet->mld2r.ngrec = htons (packet->mld2r.ngrec);
	grec->grec_nsrcs = htons (grec->grec_nsrcs);

	/* Calculate and fill in the checksum */
	packet->ip6.ip6_plen = htons (length);
	packet->mld2r.csum = htons (0);
	packet->mld2r.csum = ipv6_checksum (&packet->ip6, IPPROTO_ICMPV6, (uint8_t *) & packet->mld2r, length - sizeof (struct ip6_hbh) - sizeof (packet->routeralert));
	count++;
	dbg ("%04d.Sending2 MLDv2 Report on %s/%u, ngrec=%u, length=%u sources=%u (in last grec)\n", count, intn->name, intn->ifindex, ntohs (packet->mld2r.ngrec), length, ntohs (grec->grec_nsrcs));
	sendpacket6 (intn, (const struct ip6_hdr *) packet, length + sizeof (packet->ip6));

	/* Increase ICMP sent statistics */
	g_conf->stat_icmp_sent++;
	intn->stat_icmp_sent++;

	free (packet);
	//fprintf(stdout,"Total ICMP packets sent = %llu\n", intn->stat_icmp_sent );
	return 0;
}
