/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#ifndef __MLD_H__
#define __MLD_H__

/* Wrappers so we don't have to change the copied stuff ;) */
#define __u8 uint8_t
#define __u16 uint16_t
/* Booleans */
#define false	0
#define true	(!false)


/* Determine Endianness */
#if ! defined __LITTLE_ENDIAN_BITFIELD && ! defined __BIG_ENDIAN_BITFIELD
  #if BYTE_ORDER == LITTLE_ENDIAN
	/* 1234 machines */
	#define __LITTLE_ENDIAN_BITFIELD 1
  #elif BYTE_ORDER == BIG_ENDIAN
	/* 4321 machines */
	#define __BIG_ENDIAN_BITFIELD 1
	# define WORDS_BIGENDIAN 1
  #elif BYTE_ORDER == PDP_ENDIAN
	/* 3412 machines */
	#error PDP endianness not supported yet!
  #else
        #error unknown endianness!
  #endif
#endif  

/* Per RFC */
struct mld1
{
	__u8 type;
	__u8 code;
	__u16 csum;
	__u16 mrc;
	__u16 resv1;
	struct in6_addr mca;
};

/* The timeout for queries */
/* as per RFC3810 MLDv2 "9.2.  Query Interval" */
#define ECMH_SUBSCRIPTION_TIMEOUT	15

/* Robustness Factor, per RFC3810 MLDv2 "9.1.  Robustness Variable" */
#define ECMH_ROBUSTNESS_FACTOR		2


/* MLDv2 Report */
#ifndef ICMP6_V2_MEMBERSHIP_REPORT
#define ICMP6_V2_MEMBERSHIP_REPORT	143
#endif
/* MLDv2 Report - Experimental Code */
#ifndef ICMP6_V2_MEMBERSHIP_REPORT_EXP
#define ICMP6_V2_MEMBERSHIP_REPORT_EXP	206
#endif

/* MLDv2 Exclude/Include */

#ifndef MLD2_MODE_IS_INCLUDE
#define MLD2_MODE_IS_INCLUDE		1
#endif
#ifndef MLD2_MODE_IS_EXCLUDE
#define MLD2_MODE_IS_EXCLUDE		2
#endif
#ifndef MLD2_CHANGE_TO_INCLUDE
#define MLD2_CHANGE_TO_INCLUDE		3
#endif
#ifndef MLD2_CHANGE_TO_EXCLUDE
#define MLD2_CHANGE_TO_EXCLUDE		4
#endif
#ifndef MLD2_ALLOW_NEW_SOURCES
#define MLD2_ALLOW_NEW_SOURCES		5
#endif
#ifndef MLD2_BLOCK_OLD_SOURCES
#define MLD2_BLOCK_OLD_SOURCES		6
#endif

#ifndef MLD2_ALL_MCR_INIT
#define MLD2_ALL_MCR_INIT { { { 0xff,0x02,0,0,0,0,0,0,0,0,0,0,0,0,0,0x16 } } }
#endif

#ifndef ICMP6_ROUTER_RENUMBERING
#define ICMP6_ROUTER_RENUMBERING	138	/* router renumbering */
#endif
#ifndef ICMP6_NI_QUERY
#define ICMP6_NI_QUERY			139	/* node information request */
#endif
#ifndef ICMP6_NI_REPLY
#define ICMP6_NI_REPLY			140	/* node information reply */
#endif
#ifndef MLD_MTRACE_RESP
#define	MLD_MTRACE_RESP			200	/* Mtrace response (to sender) */
#endif
#ifndef MLD_MTRACE
#define	MLD_MTRACE			201	/* Mtrace messages */
#endif

#ifndef ICMP6_DST_UNREACH_BEYONDSCOPE
#define ICMP6_DST_UNREACH_BEYONDSCOPE	2	/* Beyond scope of source address */
#endif

#ifndef ICMP6_NI_SUCCESS
#define ICMP6_NI_SUCCESS	0	/* node information successful reply */
#endif
#ifndef ICMP6_NI_REFUSED
#define ICMP6_NI_REFUSED	1	/* node information request is refused */
#endif
#ifndef  ICMP6_NI_UNKNOWN
#define ICMP6_NI_UNKNOWN	2	/* unknown Qtype */
#endif

#ifndef ICMP6_ROUTER_RENUMBERING_COMMAND
#define ICMP6_ROUTER_RENUMBERING_COMMAND	0	/* rr command */
#endif
#ifndef ICMP6_ROUTER_RENUMBERING_RESULT
#define ICMP6_ROUTER_RENUMBERING_RESULT		1	/* rr result */
#endif
#ifndef ICMP6_ROUTER_RENUMBERING_SEQNUM_RESET
#define ICMP6_ROUTER_RENUMBERING_SEQNUM_RESET	255	/* rr seq num reset */
#endif

#ifndef ICMP6_MEMBERSHIP_QUERY
#define ICMP6_MEMBERSHIP_QUERY      130
#endif

#ifndef ICMP6_MEMBERSHIP_REPORT
#define ICMP6_MEMBERSHIP_REPORT     131
#endif

#ifndef ICMP6_MEMBERSHIP_REDUCTION
#define ICMP6_MEMBERSHIP_REDUCTION  132
#endif

#ifndef ICMP6_DST_UNREACH_NOTNEIGHBOR
#define ICMP6_DST_UNREACH_NOTNEIGHBOR 2	/* not a neighbor */
#endif

#ifdef __UCLIBC__

#ifndef IP6OPT_PADN
#define IP6OPT_PADN 1
#endif

#ifndef ip6_ext
struct ip6_ext
{
	uint8_t ip6e_nxt;
	uint8_t ip6e_len;
};
#endif
#endif

#ifdef WIN32
struct ip6_hdr
  {
    union
      {
        struct ip6_hdrctl
          {
            uint32_t ip6_un1_flow;   /* 4 bits version, 8 bits TC,
                                        20 bits flow-ID */
            uint16_t ip6_un1_plen;   /* payload length */
            uint8_t  ip6_un1_nxt;    /* next header */
            uint8_t  ip6_un1_hlim;   /* hop limit */
          } ip6_un1;
        uint8_t ip6_un2_vfc;       /* 4 bits version, top 4 bits tclass */
      } ip6_ctlun;
    struct in6_addr ip6_src;      /* source address */
    struct in6_addr ip6_dst;      /* destination address */
  };
#define ICMP6_DST_UNREACH             1
#define ICMP6_TIME_EXCEEDED           3
#define ICMP6_PARAM_PROB              4

#define ip6_vfc   ip6_ctlun.ip6_un2_vfc
#define ip6_plen  ip6_ctlun.ip6_un1.ip6_un1_plen
#define ip6_nxt   ip6_ctlun.ip6_un1.ip6_un1_nxt
#define ip6_hlim  ip6_ctlun.ip6_un1.ip6_un1_hlim

#define IP6OPT_PADN  1

/* Hop-by-Hop options header.  */
struct ip6_hbh
  {
    uint8_t  ip6h_nxt;          /* next header.  */
    uint8_t  ip6h_len;          /* length in units of 8 octets.  */
    /* followed by options */
  };

#endif


/* From linux/net/ipv6/mcast.c */

/*
 *  These header formats should be in a separate include file, but icmpv6.h
 *  doesn't have in6_addr defined in all cases, there is no __u128, and no
 *  other files reference these.
 *
 *                      +-DLS 4/14/03
 *
 * Multicast Listener Discovery version 2 headers
 * Modified as they are where not ANSI C compliant
 */
struct mld2_grec
{
	__u8 grec_type;
	__u8 grec_auxwords;
	__u16 grec_nsrcs;
	struct in6_addr grec_mca;
/*	struct in6_addr		grec_src[0]; */
};

struct mld2_report
{
	__u8 type;
	__u8 resv1;
	__u16 csum;
	__u16 resv2;
	__u16 ngrec;
/*	struct mld2_grec	grec[0]; */
};

struct mld2_query
{
	__u8 type;
	__u8 code;
	__u16 csum;
	__u16 mrc;
	__u16 resv1;
	struct in6_addr mca;
#if defined(__LITTLE_ENDIAN_BITFIELD)
	uint32_t qrv:3, suppress:1, resv2:4;
#elif defined(__BIG_ENDIAN_BITFIELD)
	uint32_t resv2:4, suppress:1, qrv:3;
#else
#error "Please fix <asm/byteorder.h>"
#endif
	__u8 qqic;
	__u16 nsrcs;
/*	struct in6_addr		srcs[0]; */
};

#define IGMP6_UNSOLICITED_IVAL	(10*HZ)
#define MLD_QRV_DEFAULT		2

#define MLD_V1_SEEN(idev) ((idev)->mc_v1_seen && \
	time_before(jiffies, (idev)->mc_v1_seen))

#define MLDV2_MASK(value, nb) ((nb)>=32 ? (value) : ((1<<(nb))-1) & (value))
#define MLDV2_EXP(thresh, nbmant, nbexp, value) \
	((value) < (thresh) ? (value) : \
	((MLDV2_MASK(value, nbmant) | (1<<(nbmant+nbexp))) << \
	(MLDV2_MASK((value) >> (nbmant), nbexp) + (nbexp))))

#define MLDV2_QQIC(value) MLDV2_EXP(0x80, 4, 3, value)
#define MLDV2_MRC(value) MLDV2_EXP(0x8000, 12, 3, value)

#define bool 	int

#define RAW_RX_BUF_SIZE 1024*1024

struct lookup
{
	unsigned int num;
	const char *desc;
};

/* Our configuration structure */
struct conf
{
	unsigned int maxinterfaces;	/* The max number of interfaces the array can hold */
	struct intnode *ints;	/* The interfaces we are watching */

	struct mc_group *groups;

	bool quit;		/* Global Quit signal */

	bool promisc;		/* Make interfaces promisc? (To be sure to receive all MLD's) */
	bool mode;		/* Streamer 0 /Client mode 1 */

	uint8_t *buffer;	/* Our buffer */
	unsigned int bufferlen;	/* Length of the buffer */

	int rawsocket;		/* Single RAW socket for sending and receiving everything */

	unsigned int stat_packets_received;	/* Number of packets received */
	unsigned int stat_packets_sent;	/* Number of packets forwarded */
	unsigned int stat_bytes_received;	/* Number of bytes received */
	unsigned int stat_bytes_sent;	/* Number of bytes forwarded */
	unsigned int stat_icmp_received;	/* Number of ICMP's received */
	unsigned int stat_icmp_sent;	/* Number of ICMP's sent */

	unsigned int mca_groups;
	unsigned int subscribers;
	cmdline_t *cmd;
#ifdef SERVER
	tuner_t *tuner_parms;
	int tuner_number;

	satellite_list_t *sat_list;
	int sat_list_num;
	int readconfig;
	pthread_t mld_timer_thread;
	pthread_t collect_stats_thread;
	pthread_t stream_tca_thread;
	pthread_t stream_tra_thread;

        int tca_id;
#endif

	pthread_mutex_t lock;
};

/* Global Stuff */
extern struct conf *g_conf;
extern struct lookup mld2_grec_types[];


const char *lookup (struct lookup *l, unsigned int num);
const char *icmpv6_type (unsigned int type);
const char *icmpv6_code (unsigned int type, unsigned int code);

uint16_t ipv6_checksum (const struct ip6_hdr *ip6, uint8_t protocol, const void *data, const uint16_t length);
void sendpacket6 (struct intnode *intn, const struct ip6_hdr *iph, const uint16_t len);
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int send_mldv2_report (struct intnode *intn, int grec_number, struct in6_addr *mcas, int srcn, struct in6_addr *sources, int mode);
int start_mld_daemon (void);

#endif
