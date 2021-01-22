/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#define MAX_DEVICES (8)	// error: expected ')' before numeric constant

typedef struct dvblo_dev {
	struct list list;

	pthread_t dvblo_recv_thread;
	char uuid[UUID_SIZE];
	int device;
	int fd;
	pthread_t dvblo_ca_thread;
	int fd_ca;
	int recv_run;
	int ci_slot;
	int ca_enable;
	
	fe_type_t type;
	recv_info_t *r;
	ci_dev_t *c;
	struct dvb_frontend_info info;
	dvblo_cacaps_t *cacaps;
	dvblo_pids_t pids;
	dvb_pid_t dstpids[RECV_MAX_PIDS];
	dvblo_sec_t sec;
	struct dvb_frontend_parameters fe_parms;
	tra_t ten;
	int tuner;
	netceiver_info_t *nci;
} dvblo_dev_t;

int dvblo_init (void);
void dvblo_exit (void);
void dvblo_handler (void);
