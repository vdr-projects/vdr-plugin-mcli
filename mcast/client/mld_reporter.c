/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#undef DEBUG
#include "headers.h"

extern pthread_mutex_t lock;
extern recv_info_t receivers;

extern int mld_start;
static pthread_t mld_send_reports_thread;
static char iface[IFNAMSIZ];

static int find_mcg_in_mld_mcas (struct in6_addr *mld_mca, int len, struct in6_addr *mcg)
{
	int i;

	for (i = 0; i < len; i++) {
		if (!memcmp (mld_mca + i, mcg, sizeof (struct in6_addr))) {
			return 1;
		}
	}
	return 0;
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

typedef struct {
	struct in6_addr *mld_mca_add;
	struct in6_addr *mld_mca_drop;
} mld_reporter_context_t;

static void clean_mld_send_reports_thread(void *p)
{
	mld_reporter_context_t *c=(mld_reporter_context_t*)p;
	if(c->mld_mca_add) {
		free(c->mld_mca_add);
	}
	if(c->mld_mca_drop) {
		free(c->mld_mca_drop);
	}
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void *mld_send_reports (void *arg)
{
	recv_info_t *receivers = (recv_info_t *) arg;

	int grec_num_drop;
	int grec_num_add;
	pid_info_t *p;
	pid_info_t *ptmp;
	recv_info_t *r;
	int maxpids=128;
	mld_reporter_context_t c;
	memset(&c, 0, sizeof(mld_reporter_context_t));

	c.mld_mca_add=(struct in6_addr *)malloc(maxpids*sizeof(struct in6_addr));
	if (!c.mld_mca_add)
		err ("mld_send_reports: out of memory\n");
	c.mld_mca_drop=(struct in6_addr *)malloc(maxpids*sizeof(struct in6_addr));
	if (!c.mld_mca_drop)
		err ("mld_send_reports: out of memory\n");
	
	pthread_cleanup_push (clean_mld_send_reports_thread, &c);
	
	struct intnode *intn = int_find_name (iface);
	
	if( !c.mld_mca_add || !c.mld_mca_drop) {
		err ("Cannot get memory for add/drop list\n");
	}
	mld_start=1;	
	while (mld_start) {
		grec_num_drop=0;
		pthread_mutex_lock (&lock);
		
		int pids=count_all_pids(receivers);
		if(pids>maxpids) {
			maxpids=pids;
			c.mld_mca_add=(struct in6_addr *)realloc(c.mld_mca_add, pids*sizeof(struct in6_addr));
			if (!c.mld_mca_add)
				err ("mld_send_reports: out of memory\n");
			c.mld_mca_drop=(struct in6_addr *)realloc(c.mld_mca_drop, pids*sizeof(struct in6_addr));
			if (!c.mld_mca_drop)
				err ("mld_send_reports: out of memory\n");
		}

		//Send listener reports for all recently dropped MCGs
		DVBMC_LIST_FOR_EACH_ENTRY (r, &receivers->list, recv_info_t, list) {
			DVBMC_LIST_FOR_EACH_ENTRY_SAFE (p, ptmp, &r->slots.list, pid_info_t, list) {
				// prevent a somewhere running mcg on any device to be dropped and prevent to drop same mcg multiple times
				if (!p->run) {
					if ( p->dropped && !find_any_slot_by_mcg (receivers, &p->mcg) && !find_mcg_in_mld_mcas (c.mld_mca_drop, grec_num_drop, &p->mcg)) {
						memcpy (c.mld_mca_drop + grec_num_drop++, &p->mcg.s6_addr, sizeof (struct in6_addr));
						p->dropped--;
#ifdef DEBUG	
						char host[INET6_ADDRSTRLEN];
						inet_ntop (AF_INET6, p->mcg.s6_addr, (char *) host, INET6_ADDRSTRLEN);
						dbg ("DROP_GROUP %d %s\n", grec_num_drop, host);
#endif
					} else {
						dvbmc_list_remove(&p->list);
						free(p);
					}
				}
			}
		}
		if(grec_num_drop > maxpids) {
			err ("Wrong number of pids: %d>%d\n", grec_num_drop, maxpids);
		}
		grec_num_add=0;
		//Send listener reports for all current MCG in use
		DVBMC_LIST_FOR_EACH_ENTRY (r, &receivers->list, recv_info_t, list) {
			DVBMC_LIST_FOR_EACH_ENTRY (p, &r->slots.list, pid_info_t, list) {
				if (p->run && !find_mcg_in_mld_mcas (c.mld_mca_add, grec_num_add, &p->mcg)) {
					memcpy (c.mld_mca_add + grec_num_add++, p->mcg.s6_addr, sizeof (struct in6_addr));
#ifdef DEBUG
					char host[INET6_ADDRSTRLEN];
					inet_ntop (AF_INET6, &p->mcg.s6_addr, (char *) host, INET6_ADDRSTRLEN);
					dbg ("ADD_GROUP %d %s\n", grec_num_add, host);
#endif
				}
			}
		}

		if(grec_num_add > maxpids) {
			err ("Wrong number of pids: %d>%d\n", grec_num_add, maxpids);
		}

		pthread_mutex_unlock (&lock);

		if (intn && intn->mtu) {
			if (grec_num_drop) {
				send_mldv2_report (intn, grec_num_drop, c.mld_mca_drop, 0, NULL, MLD2_MODE_IS_INCLUDE);
			}
			if (grec_num_add) {
				send_mldv2_report (intn, grec_num_add, c.mld_mca_add, 0, NULL, MLD2_MODE_IS_EXCLUDE);
			}
		}
		usleep (REP_TIME);
		pthread_testcancel();
	}
	pthread_cleanup_pop (1);
	return NULL;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int mld_client_init (char *intf)
{
	if(intf) {
		strcpy(iface, intf);
	} else {
		iface[0]=0;
	}
	
	if (!strlen (iface)) {
		struct intnode *intn = int_find_first ();
		if (intn) {
			strcpy (iface, intn->name);
		} else {
			warn ("Cannot find any usable network interface\n");
			return -1;
		}
	}

#if ! (defined WIN32 || defined APPLE)
	g_conf->rawsocket = socket (PF_PACKET, SOCK_DGRAM, htons (ETH_P_ALL));
#endif
#ifdef WIN32
	g_conf->rawsocket = socket (PF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
#endif
#ifdef APPLE
	g_conf->rawsocket = socket (PF_INET6, SOCK_RAW, IPPROTO_HOPOPTS);
#endif
	if (g_conf->rawsocket < 0) {
		warn ("Cannot get a packet socket\n");
		return -1;
	}
#ifdef WIN32
	#define	IPV6_HDRINCL	2 
	DWORD n=1;
	if (setsockopt (g_conf->rawsocket, IPPROTO_IPV6, IPV6_HDRINCL, (char *)&n, sizeof (n)) < 0) {
		err ("setsockopt IPV6_HDRINCL");
	}
	int idx;
	if ((idx = if_nametoindex (iface))>0) {
		int ret=setsockopt (g_conf->rawsocket, IPPROTO_IPV6, IPV6_MULTICAST_IF, (_SOTYPE)&idx, sizeof (idx));
		if(ret<0) {
			warn("setsockopt for IPV6_MULTICAST_IF failed with %d error %s (%d)\n",ret,strerror (errno), errno);
		}
	}
#endif
	pthread_create (&mld_send_reports_thread, NULL, mld_send_reports, &receivers);
	return 0;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void mld_client_exit (void)
{
	if(g_conf) {
		mld_start=0;
		if(pthread_exist(mld_send_reports_thread)) {
			if(pthread_exist(mld_send_reports_thread) && !pthread_cancel (mld_send_reports_thread)) {
				pthread_join (mld_send_reports_thread, NULL);
			}
		}
#if 0
		struct intnode *intn;
		unsigned int i;
		for (i = 0; i < g_conf->maxinterfaces; i++) {
			intn = &g_conf->ints[i];
			if (intn->mtu == 0)
				continue;
			int_destroy (intn);
		}
#endif
		closesocket(g_conf->rawsocket);
	}
}
