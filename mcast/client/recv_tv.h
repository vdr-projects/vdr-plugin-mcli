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

#ifndef __RECV_TV_H__
#define __RECV_TV_H__

#define REP_TIME 1000000
#define MAX_DROP_NUM 5
#define RECV_MAX_PIDS 256

//typedef struct recv_info recv_info_t;

typedef struct {
  int pid;
  int id;
  int priority;
#if 1
  int re;
#endif
} dvb_pid_t;

typedef struct pid_info
{
	struct list list;
	UDPContext *s;
	dvb_pid_t pid;
	struct in6_addr mcg;
	recv_info_t *recv;
	pthread_t recv_ts_thread;
	int run;
	int dropped;
	int cont_old;
} pid_info_t;

struct recv_info
{
	struct list list;
	recv_info_t *head;
	pid_info_t slots;
	int lastalloc;
	pthread_t recv_ten_thread;
	struct in6_addr mcg;
	int ten_run;

	dvb_pid_t pids[RECV_MAX_PIDS];
	int pidsnum;
	recv_sec_t sec;
	struct dvb_frontend_parameters fe_parms;

	recv_festatus_t fe_status;
		
	int (*handle_ten) (tra_t *ten, void *context);
	void *handle_ten_context;
	
	int (*handle_ts) (unsigned char *buffer, size_t len, void *context);
	void *handle_ts_context;
};

// Internal Stuff
int recv_redirect (recv_info_t * r, struct in6_addr mcg);
int count_all_pids (recv_info_t * receivers);
int count_receivers(recv_info_t *receivers);

// PID-Handling
DLL_SYMBOL int recv_pid_add (recv_info_t * r, dvb_pid_t *pid);
DLL_SYMBOL int recv_pid_del (recv_info_t * r, int pid);
DLL_SYMBOL int recv_pids (recv_info_t * r, dvb_pid_t *pids);
DLL_SYMBOL int recv_pids_get (recv_info_t *r, dvb_pid_t *pids);
DLL_SYMBOL int recv_show_all_pids (recv_info_t * receivers);
void recv_show_pids(recv_info_t *r);

// Complete Tune
DLL_SYMBOL int recv_tune (recv_info_t * r, fe_type_t type, int satpos, recv_sec_t *sec, struct dvb_frontend_parameters *fe_parms, dvb_pid_t *pids);

// Receiver Handling
DLL_SYMBOL recv_info_t *recv_add (void);
DLL_SYMBOL void recv_del (recv_info_t *r);
DLL_SYMBOL int recv_stop (recv_info_t * r);
DLL_SYMBOL int register_ts_handler (recv_info_t * r, int (*p)(unsigned char *, size_t, void *), void *c);

// Module global functions
DLL_SYMBOL int recv_init(char *intf, int p);
DLL_SYMBOL int recv_exit(void);


int find_any_slot_by_mcg (recv_info_t * receivers, struct in6_addr *mcg);

#endif
