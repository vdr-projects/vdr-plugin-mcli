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

//#define DEBUG 1
#include "headers.h"
#if ! defined WIN32 || defined __CYGWIN__
#define RT
#endif

#define RE 1

#if defined(RE)
int set_redirected(recv_info_t *r, int sid);
int check_if_already_redirected(recv_info_t *r, int sid);
#endif

recv_info_t receivers;
pthread_mutex_t lock;

int mld_start=0;

int port=23000;
char iface[IFNAMSIZ];

static pthread_t recv_tra_thread;
static pthread_t recv_tca_thread;

#if ! defined WIN32 || defined __CYGWIN__
static void sig_handler (int signal)
{
	dbg ("Signal: %d\n", signal);
	
	switch (signal) {
	case SIGUSR1:
		recv_show_all_pids (&receivers);
		break;
	}
}
#endif

#ifdef MULTI_THREAD_RECEIVER
static void clean_recv_ts_thread (void *arg)
{
	pid_info_t *p = (pid_info_t *) arg;
#ifdef DEBUG
	dbg ("Stop stream receiving for pid %d\n", p->pid.pid);
#endif

	if (p->s) {
		udp_close (p->s);
	}
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void *recv_ts (void *arg)
{
	unsigned char buf[32712];
	unsigned char *ptr;
	int n, res;
	int cont_old = -1;

	pid_info_t *p = (pid_info_t *) arg;
	recv_info_t *r = p->recv;

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

	pthread_cleanup_push (clean_recv_ts_thread, p);
#ifdef DEBUG	
	char addr_str[INET6_ADDRSTRLEN];
	inet_ntop (AF_INET6, p->mcg.s6_addr, addr_str, INET6_ADDRSTRLEN);
	dbg ("Start stream receiving for %s on port %d %s\n", addr_str, port, iface);
#endif
	p->s = client_udp_open (&p->mcg, port, iface);
	if (!p->s) {
		warn ("client_udp_open error !\n");
	} else {
		p->run = 1;
	}
	while (p->run>0) {
		n = udp_read (p->s, buf, sizeof (buf), 1000, NULL);
		if (n >0 ) {
			ptr = buf;
			if (n % 188) {
				warn ("Mcli::%s: Received %d bytes is not multiple of 188!\n", __FUNCTION__, n);
 			}
			int i;
			for (i = 0; i < (n / 188); i++) {
				unsigned char *ts = buf + (i * 188);
				int adaption_field = (ts[3] >> 4) & 3;
				int cont = ts[3] & 0xf;
				int pid = ((ts[1] << 8) | ts[2]) & 0x1fff;
				int transport_error_indicator = ts[1]&0x80;
	
				if (pid != 8191 && (adaption_field & 1) && (((cont_old + 1) & 0xf) != cont) && cont_old >= 0) {
					warn ("Mcli::%s: Discontinuity on receiver %p for pid %d: %d->%d at pos %d/%d\n", __FUNCTION__, r, pid, cont_old, cont, i, n / 188);
 				}
				if (transport_error_indicator) {
					warn ("Mcli::%s: Transport error indicator set on receiver %p for pid %d: %d->%d at pos %d/%d\n", __FUNCTION__, r, pid, cont_old, cont, i, n / 188);
 				}
				cont_old = cont;
 			}
			if(r->handle_ts) {
				while (n) {
					res = r->handle_ts (ptr, n, r->handle_ts_context);
					if (res != n) {
						warn ("Mcli::%s: Not same amount of data written: res:%d<=n:%d\n", __FUNCTION__, res, n);
					}
					if (res < 0) {
						warn ("Mcli::%s: write of %d bytes returned %d\n", __FUNCTION__, n, res);
						perror ("Write failed");
						break;
					} else {
						ptr += res;
						n -= res;
					}
				}
			}
 		}
		pthread_testcancel();
 	}
	pthread_cleanup_pop (1);

	return NULL;
 }

#else
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void recv_ts_func (unsigned char *buf, int n, void *arg) {
	static long int warn_count = 0;
	static long int warn_count_last = 0;
	static time_t warn_time_last;
	time_t warn_time_now;
	int warn_show = 0;
	double warn_time_diff = 0;

	if (n >0 ) {
		pid_info_t *p = (pid_info_t *) arg;
		recv_info_t *r = p->recv;
		int i;
		for (i = 0; i < n; i += 188) {
			unsigned char *ts = buf + i;
			int adaption_field = (ts[3] >> 4) & 3;
			int cont = ts[3] & 0xf;
			int pid = ((ts[1] << 8) | ts[2]) & 0x1fff;
			int transport_error_indicator = ts[1]&0x80;

			if (pid != 8191 && (adaption_field & 1) && (((p->cont_old + 1) & 0xf) != cont) && p->cont_old >= 0) {
				warn_show = 0;
				time(&warn_time_now);
				if (warn_count == 0) { // 1st occurance
					warn_show = 1; // always display 1st
				} else {
					warn_time_diff = difftime(warn_time_now, warn_time_last);
					if (warn_time_diff >= 2) {
						warn_show = 1; // display after 2 sec again
					};
				};
				warn_count++;
				if (warn_show > 0) {
					time(&warn_time_last);
					if ((warn_count - warn_count_last) > 1) {
						warn ("Mcli::%s: Discontinuity on receiver messages suppressed in %ld seconds: %ld\n", __FUNCTION__, (long int) warn_time_diff, (warn_count - warn_count_last));
					};
					warn ("Mcli::%s: Discontinuity on receiver %p for pid %d: %d->%d at pos %d/%d\n", __FUNCTION__, r, pid, p->cont_old, cont, i / 188, n / 188);
					warn_count_last = warn_count;
				};
			}
			if (transport_error_indicator) {
				warn ("Mcli::%s: Transport error indicator set on receiver %p for pid %d: %d->%d at pos %d/%d\n", __FUNCTION__, r, pid, p->cont_old, cont, i / 188, n / 188);
			}
			p->cont_old = cont;
		}
		if (i != n) {
			warn ("Mcli::%s: Received %d bytes is not multiple of 188!\n", __FUNCTION__, n);
		}
		if(r->handle_ts) {
			while (n) {
				int res = r->handle_ts (buf, n, r->handle_ts_context);
				if (res != n) {
					warn ("Mcli::%s: Not same amount of data written: res:%d<=n:%d\n", __FUNCTION__, res, n);
				}
				if (res < 0) {
					warn ("Mcli::%s: write of %d bytes returned %d\n", __FUNCTION__, n, res);
					perror ("Write failed");
					break;
				} else {
					buf += res;
					n -= res;
				}
			}
		}
	}
}
#endif
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int register_ts_handler (recv_info_t * r, int (*p)(unsigned char *, size_t, void *), void *c)
{
	r->handle_ts=(int (*)(unsigned char *buffer, size_t len, void *context))p;
	r->handle_ts_context=c;
	return 0;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static pid_info_t *find_slot_by_pid (recv_info_t * r, int pid, int id)
{
	pid_info_t *slot;
	DVBMC_LIST_FOR_EACH_ENTRY (slot, &r->slots.list, pid_info_t, list) {
		if (slot->run && slot->pid.pid == pid  && (id == -1 || slot->pid.id == id)) {
			return slot;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static pid_info_t *find_slot_by_mcg (recv_info_t * r, struct in6_addr *mcg)
{
	pid_info_t *slot;
	
	DVBMC_LIST_FOR_EACH_ENTRY (slot, &r->slots.list, pid_info_t, list) {
		if (slot->run && !memcmp (&slot->mcg, mcg, sizeof (struct in6_addr))) {
			return slot;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int find_any_slot_by_mcg (recv_info_t * receivers, struct in6_addr *mcg)
{
	recv_info_t *r;
	int ret=0;
	
	DVBMC_LIST_FOR_EACH_ENTRY (r, &receivers->head->list, recv_info_t, list) {
		pid_info_t *slot = find_slot_by_mcg (r, mcg);
		if(slot) {
			ret++;
		}
	}
	return ret;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int count_receivers(recv_info_t *receivers)
{
	int ret=0;
	struct list *pos;
	
	DVBMC_LIST_FOR_EACH (pos, &receivers->list) {
		ret++;
	}
	return ret;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static int count_pids(recv_info_t *r)
{
	int ret=0;
	struct list *pos;
	
	DVBMC_LIST_FOR_EACH (pos, &r->slots.list) {
		ret++;
	}
	return ret;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int count_all_pids (recv_info_t * receivers)
{
	int ret=0;
	recv_info_t *r;

	DVBMC_LIST_FOR_EACH_ENTRY (r, &receivers->head->list, recv_info_t, list) {
		ret += count_pids(r);
	}
	return ret;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void recv_show_pids(recv_info_t *r)
{
	pid_info_t *slot;
        char addr_str[INET6_ADDRSTRLEN];
	inet_ntop (AF_INET6, r->mcg.s6_addr, addr_str, INET6_ADDRSTRLEN);
                	
	info("pids on receiver %p (%s):\n",r, addr_str);
	DVBMC_LIST_FOR_EACH_ENTRY (slot, &r->slots.list, pid_info_t, list) {
		info("%d,", slot->pid.pid);
	}
	info("\n");
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int recv_show_all_pids (recv_info_t * receivers)
{
	int ret=0;
	recv_info_t *r;
	DVBMC_LIST_FOR_EACH_ENTRY (r, &receivers->head->list, recv_info_t, list) {
		recv_show_pids(r);
	}
	return ret;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void deallocate_slot (recv_info_t * r, pid_info_t *p)
{
	int nodrop=0;
	
#ifdef MULTI_THREAD_RECEIVER
	if (pthread_exist(p->recv_ts_thread)) {
#else
	if (p->run) {

#endif		//info ("Deallocating PID %d from slot %p\n", p->pid.pid, p);
		p->run = 0;

		//Do not leave multicast group if there is another dvb adapter using the same group
		if (find_any_slot_by_mcg (r, &p->mcg)) {
			dbg ("MCG is still in use not dropping\n");
			p->s->is_multicast = 0;
			nodrop=1;
		} 

#ifdef MULTI_THREAD_RECEIVER
		pthread_join (p->recv_ts_thread, NULL);
#else
		udp_close_buff(p->s);
#endif
		p->dropped = MAX_DROP_NUM;
	}
	//printf("NO DROP: %d\n",nodrop);
	if(!mld_start || nodrop) {
		dvbmc_list_remove(&p->list);
		free(p);
	}
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static pid_info_t *allocate_slot (recv_info_t * r, struct in6_addr *mcg, dvb_pid_t *pid)
{
	pid_info_t *p = (pid_info_t *)malloc(sizeof(pid_info_t));
	if(!p) {
		err ("Cannot get memory for pid\n");
	}
	
	dbg ("Allocating new PID %d to Slot %p\n", pid->pid, p);
	
	memset(p, 0, sizeof(pid_info_t)); 
	
	p->cont_old = -1;
	p->mcg = *mcg;
	mcg_set_pid (&p->mcg, pid->pid);
#if defined(RE)
	if (!check_if_already_redirected(r, pid->id)) {
		//printf("PID %d not red. ===> SETTING ID to %d\n",pid->pid,pid->id);	
		mcg_set_id (&p->mcg, pid->id);
		mcg_set_priority(&p->mcg, pid->priority);
	} else {
		set_redirected(r, pid->id);	
		//printf("send pid %d to noid mcg !\n",pid->pid);
		mcg_set_id(&p->mcg, 0);
		mcg_set_priority(&p->mcg, 0);
	}
	//mcg_set_id(&p->mcg,pid->id);
#else
	mcg_set_id (&p->mcg, pid->id);
	mcg_set_priority(&p->mcg, pid->priority);
#endif


#ifdef DEBUG
	print_mcg (&p->mcg);
#endif
	p->pid = *pid;
	p->recv = r;
#ifdef MULTI_THREAD_RECEIVER
	int ret = pthread_create (&p->recv_ts_thread, NULL, recv_ts, p);
	while (!ret && !p->run) {
		usleep (10000);
	}
	if (ret) {
		err ("pthread_create failed with %d\n", ret);
#else
	p->cont_old=-1;
	p->s = client_udp_open_cb (&p->mcg, port, iface, recv_ts_func, p);
	if (!p->s) {
		warn ("client_udp_open error !\n");
		return 0;
#endif
	} else {
		p->run = 1;
		dvbmc_list_add_head (&r->slots.list, &p->list);
	}

	return p;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void stop_ten_receive (recv_info_t * r)
{
	dbg ("->>>>>>>>>>>>>>>>>stop_ten_receive on receiver %p\n",r);
	if (pthread_exist(r->recv_ten_thread) &&  r->ten_run) {
		dbg ("cancel TEN receiver %p %ld\n", r, r->recv_ten_thread);
		
		r->ten_run=0;
		pthread_mutex_unlock (&lock);
		do {
			dbg ("wait TEN stop receiver %p %ld\n", r, r->recv_ten_thread);
			usleep(10000);
		} while (!r->ten_run);
		pthread_mutex_lock (&lock);
		r->ten_run=0;
		dbg ("cancel TEN done receiver %p\n", r);
		pthread_detach (r->recv_ten_thread);
		pthread_null(r->recv_ten_thread);
	}
}


//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void start_ten_receive (recv_info_t * r)
{
	if (r->pidsnum && !pthread_exist(r->recv_ten_thread)) {
#ifdef DEBUG
		char host[INET6_ADDRSTRLEN];
		inet_ntop (AF_INET6, &r->mcg, (char *) host, INET6_ADDRSTRLEN);

		dbg ("Start TEN for receiver %p %s\n", r, host);
#endif
		r->ten_run = 0;

		int ret = pthread_create (&r->recv_ten_thread, NULL, recv_ten, r);
		while (!ret && !r->ten_run) {
			dbg ("wait TEN startup receiver %p %ld\n", r, r->recv_ten_thread);
			usleep (10000);
		}
		if (ret) {
			err ("pthread_create failed with %d\n", ret);
		}
	}
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static int cmppids(const void *p1, const void *p2)
{
	dvb_pid_t *pid1=(dvb_pid_t *)p1;
	dvb_pid_t *pid2=(dvb_pid_t *)p2;

	if(pid1->pid == pid2->pid) {
		return pid1->id < pid2->id;
	}
	return pid1->pid < pid2->pid;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void update_mcg (recv_info_t * r, int handle_ten)
{
	int i;
	pid_info_t *p;
	pid_info_t *ptmp;
	
	if(handle_ten) {
		if(r->pidsnum) {
			start_ten_receive(r);
		} else {
			stop_ten_receive(r);	
		}
	}
	dbg("update_mcg(%p, %d)\n", r, handle_ten);
	qsort(r->pids, r->pidsnum, sizeof(dvb_pid_t), cmppids);

	DVBMC_LIST_FOR_EACH_ENTRY_SAFE (p, ptmp, &r->slots.list, pid_info_t, list) {
		//dbg ("DVBMC_LIST_FOR_EACH_ENTRY_SAFE: %p\n", p);
		if(p->run) {
			int found_pid = 0;
			for (i = 0;  i < r->pidsnum; i++) {
				// pid already there without id but now also with id required
				if (r->pids[i].pid == p->pid.pid && r->pids[i].id && !p->pid.id) {
					found_pid = 0;
					break;
				}
				if (r->pids[i].pid == p->pid.pid && r->pids[i].id == p->pid.id) {
					found_pid = 1;
				}
			}
			if (!found_pid) {
				deallocate_slot (r, p);
			}
		}
	}

	for (i = 0; i < r->pidsnum; i++) {
		unsigned int pid = r->pids[i].pid;
		if (!find_slot_by_pid (r, pid, -1)) { //pid with any id there?
			allocate_slot (r, &r->mcg, r->pids+i);
		}
	}


}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void stop_receive (recv_info_t * r, int mode)
{
	dbg ("stop_receive on receiver %p mode %d\n",r, mode);
	int pidsnum=r->pidsnum;
	//Remove all PIDs
	r->pidsnum = 0;
	update_mcg (r, mode);
	r->pidsnum=pidsnum;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef RE
#if 0
static int find_redirected_sid (recv_info_t * r, int id)
{
	pid_info_t *slot;
	DVBMC_LIST_FOR_EACH_ENTRY (slot, &r->slots.list, pid_info_t, list) {
		if (slot->pid.id == id && slot->pid.re) {
			return 1;
		}
	}

	return 0;
}
#endif

int check_if_already_redirected(recv_info_t *r, int sid)
{
	int i;
	for (i = 0; i < r->pidsnum; i++) {
		//printf("PID %d SID %d RE %d\n",r->pids[i].pid, r->pids[i].id, r->pids[i].re);
		if (r->pids[i].re && r->pids[i].id == sid) {
			return 1;
		}
	}

	return 0;
}

int check_if_sid_in(recv_info_t *r, int sid)
{
	int i;
	for (i = 0; i < r->pidsnum; i++) {
		//printf("PID %d SID %d RE %d\n",r->pids[i].pid, r->pids[i].id, r->pids[i].re);
		if (r->pids[i].id == sid) {
//			printf("%s: SID in %d!\n",__func__,sid);
			return 1;
		}
	}

	return 0;
}

int set_redirected(recv_info_t *r, int sid)
{
	int i;
	for (i = 0; i < r->pidsnum; i++) {
		if (r->pids[i].id == sid)
			r->pids[i].re=1;
	}

	return 0;
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int stop_sid_mcgs(recv_info_t *r, int sid)
{
        pid_info_t *p;   
        pid_info_t *ptmp;

        DVBMC_LIST_FOR_EACH_ENTRY_SAFE (p, ptmp, &r->slots.list, pid_info_t, list) {
                if(p->run) {
                        if (p->pid.pid && p->pid.id == sid) {
                                //info ("Deallocating PID %d ID %d RE %d from slot %p\n", p->pid.pid,p->pid.id,p->pid.re, p);
                                deallocate_slot (r, p);
                        }
                }
        }        
                 
        return 0;
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int rejoin_mcgs(recv_info_t *r, int sid)
{

        int i;
        for (i = 0; i < r->pidsnum; i++) {
                unsigned int pid = r->pids[i].pid;
                unsigned int id = r->pids[i].id;
                if (!find_slot_by_pid (r, pid, id) && id == sid) {
                        char addr_str[INET6_ADDRSTRLEN];
                        inet_ntop (AF_INET6, &r->mcg, addr_str, INET6_ADDRSTRLEN);
                        //info ("Rejoin mcg %s with no ID (PID %d ID %d RE %d)...\n", addr_str, pid, id, r->pids[i].re);
                        allocate_slot (r, &r->mcg, r->pids+i);
                }
        }

        return 0;
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#endif
int recv_redirect (recv_info_t * r, struct in6_addr mcg)
{
	int ret = 0;

	pthread_mutex_lock (&lock);
	dbg ("\n+++++++++++++\nIn redirect for receiver %p\n", r);
#if 0
	char addr_str[INET6_ADDRSTRLEN];
	inet_ntop (AF_INET6, &r->mcg, addr_str, INET6_ADDRSTRLEN);
	info ("Redirect to ===>  %s\n",addr_str);
#endif	
	int sid;
	mcg_get_id(&mcg,&sid);
	mcg_set_id(&mcg,0);

	//printf("SID in: %d\n",sid);

	if (!sid || ( !check_if_already_redirected(r, sid) && check_if_sid_in(r, sid)) ) {
		if (sid == 0) {
			stop_receive (r, 0);
			r->mcg = mcg;
			update_mcg (r, 0);
			ret = 1;
		} else {
			//stop sid mcgs
			stop_sid_mcgs(r, sid);
			set_redirected(r, sid);         
			//start new mcgs with no sid
			rejoin_mcgs(r, sid);				
		}
	} 
	
	dbg ("Redirect done for receiver %p\n", r);
	pthread_mutex_unlock (&lock);

	return ret;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int recv_stop (recv_info_t * r)
{
	pthread_mutex_lock (&lock);
	stop_receive (r, 1);
	pthread_mutex_unlock (&lock);
	return 0;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int recv_count_pids(recv_info_t * r)
{
	int i;
	for (i=0; r->pids[i].pid!=-1; i++);
	return i;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static int recv_copy_pids(dvb_pid_t *dst, dvb_pid_t *src)
{
	int i;
	for (i=0; (src[i].pid!=-1) && (i<(RECV_MAX_PIDS-1)); i++) {
		dst[i]=src[i];
	}
	if(i==(RECV_MAX_PIDS-1)) {
		warn("Cannot receive more than %d pids\n", RECV_MAX_PIDS-1);
	}
	return i;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int recv_pids (recv_info_t *r, dvb_pid_t *pids)
{
	pthread_mutex_lock (&lock);
	if(pids) {
		r->pidsnum=recv_copy_pids(r->pids, pids);
	}
	update_mcg(r, 1);
	pthread_mutex_unlock (&lock);
	return 0;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int recv_pids_get (recv_info_t *r, dvb_pid_t *pids)
{
	pthread_mutex_lock (&lock);
	if(pids) {
		memcpy(pids, r->pids, sizeof(dvb_pid_t)*r->pidsnum);
		pids[r->pidsnum].pid=-1;
	}
	pthread_mutex_unlock (&lock);
	return r->pidsnum;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int recv_pid_add (recv_info_t * r, dvb_pid_t *pid)
{
	int ret=0;

	pthread_mutex_lock (&lock);
	pid_info_t *p=find_slot_by_pid (r, pid->pid, pid->id);
	if(!p && (r->pidsnum < (RECV_MAX_PIDS-2))) {
#if defined(RE)
		r->pids[r->pidsnum].re = 0;
#endif
		r->pids[r->pidsnum]=*pid;
		r->pids[++r->pidsnum].pid=-1;
		update_mcg(r, 1);
		ret = 1;
	}
	pthread_mutex_unlock (&lock);
	
	return ret;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int recv_pid_del (recv_info_t * r, int pid)
{
	int i;
	int ret=0;
	
	pthread_mutex_lock (&lock);
	if(pid>=0) {
		for (i = 0; i < r->pidsnum; i++) {
			if(r->pids[i].pid==pid || ret) {
				r->pids[i]=r->pids[i+1];
				ret=1;
			}
		}
		if(ret) {
			 r->pidsnum--;
			 update_mcg(r, 1);
		}
	} else {
		r->pids[0].pid=-1;
		r->pidsnum=0;
		update_mcg(r, 1);
	}
	pthread_mutex_unlock (&lock);
	
	return ret;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int recv_tune (recv_info_t * r, fe_type_t type, int satpos, recv_sec_t *sec, struct dvb_frontend_parameters *fe_parms, dvb_pid_t *pids)
{
	pthread_mutex_lock (&lock);
	dbg ("kick_tune receiver %p\n", r);

	stop_receive (r, 1);
	if(fe_parms) {
		r->fe_parms=*fe_parms;
	}
	if(sec) {
		r->sec=*sec;
	}
	if(pids) {
		r->pidsnum=recv_copy_pids(r->pids, pids);
	}

	fe_parms_to_mcg (&r->mcg, STREAMING_PID, type, &r->sec, &r->fe_parms, 0);
	mcg_set_satpos (&r->mcg, satpos);

	update_mcg (r, 1);

	pthread_mutex_unlock (&lock);
	dbg ("kick_tune done receiver %p\n", r);
	return 0;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

recv_info_t *recv_add (void)
{
	recv_info_t *r=(recv_info_t *)malloc(sizeof(recv_info_t));
	if(!r) {
		err ("Cannot get memory for receiver\n");
	}
	memset (r, 0, sizeof (recv_info_t));
	r->head=&receivers;
	dvbmc_list_init (&r->slots.list);
	pthread_mutex_lock (&lock);
	dvbmc_list_add_head(&receivers.list, &r->list);
	pthread_mutex_unlock (&lock);
	return r;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void recv_del (recv_info_t *r)
{
	pthread_mutex_lock (&lock);
	stop_receive (r, 1);
	dvbmc_list_remove(&r->list);
	pthread_mutex_unlock (&lock);
	free(r);
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int recv_init(char *intf, int p)
{
	LIBXML_TEST_VERSION;
#ifdef WIN32
	WSADATA wsaData;
	if (WSAStartup (MAKEWORD (2, 2), &wsaData) != 0) {
		err ("WSAStartup failed\n");
	}
#endif

	if(intf) {
		strcpy(iface, intf);
	} else {
		iface[0]=0;
	}
	if(p) {
		port=p;
	}

	g_conf = (struct conf*) malloc (sizeof (struct conf));
	if (!g_conf) {
		err ("Cannot get memory for configuration\n");
		exit (-1);
	}

	memset (g_conf, 0, sizeof (struct conf));
	update_interfaces (NULL);

	if (!strlen (iface)) {
		struct intnode *intn = int_find_first ();
		if (intn) {
			strcpy (iface, intn->name);
		} else {
			warn ("Cannot find any usable network interface\n");
			if(g_conf->ints) {
				free (g_conf->ints);
			}
			#ifdef PTW32_STATIC_LIB
				pthread_win32_process_detach_np();
			#endif
			free(g_conf);	
			return -1;
		}
	}
	
	dvbmc_list_init (&receivers.list);
	pthread_mutex_init (&lock, NULL);
	receivers.head=&receivers;
#if ! defined WIN32 || defined __CYGWIN__
	signal (SIGUSR1, &sig_handler);
#endif
#ifdef PTW32_STATIC_LIB
	pthread_win32_process_attach_np();
#endif
	pthread_create (&recv_tra_thread, NULL, recv_tra, NULL);
	pthread_create (&recv_tca_thread, NULL, recv_tca, NULL);

	return 0;	
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int recv_exit(void)
{
	recv_info_t *r;
	recv_info_t *rtmp;
	if(pthread_exist(recv_tra_thread) && !pthread_cancel (recv_tra_thread)) {
		pthread_join (recv_tra_thread, NULL);
	}
	if(pthread_exist(recv_tca_thread) && !pthread_cancel (recv_tca_thread)) {
		pthread_join (recv_tca_thread, NULL);
	}
	DVBMC_LIST_FOR_EACH_ENTRY_SAFE (r, rtmp, &receivers.head->list, recv_info_t, list) {
		recv_del(r);
	}
#if ! defined WIN32 || defined __CYGWIN__
	signal (SIGUSR1, NULL);
#endif
	g_conf->maxinterfaces=0;
	if(g_conf->ints) {
		free (g_conf->ints);
	}
#ifdef PTW32_STATIC_LIB
	pthread_win32_process_detach_np();
#endif
	free(g_conf);
	xmlCleanupParser ();
	xmlMemoryDump ();
	return 0;
}
