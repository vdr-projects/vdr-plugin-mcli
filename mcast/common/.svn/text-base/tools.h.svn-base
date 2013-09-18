/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#ifndef __TOOLS_H__
#define __TOOLS_H__

#define PID_MASK 0x1fff
#define NOPID_MASK 0xe000

#define MC_PREFIX 0xff18

#define NO_SAT_POS 0xfff

// value from sat_resolved
#define NOT_RESOLVED -1       // Not run through mcg_is_equivalent/satellite_resolver
#define NOT_SUPPORTED -2      // requested position not available
#define LEGACY_DISEQC -3


#define COMPRESSION_ON 1
#define COMPRESSION_OFF 0

struct lookup_dvb_t_fec
{
	int fec_hp;
	int fec_lp;
	int val;
};

typedef struct
{
	char *name;
	int value;
} Param;

typedef enum
{
	STREAMING_TCA = 1,
	STREAMING_TRA = 2,
	STREAMING_PID = 3,
	STREAMING_TEN = 4,
	STREAMING_LOG = 5,
} streaming_group_t;


// 8=max. tuner slots (some safety)
#define MAX_TUNER_CACHE 8

// contains parsed/cached FE params


struct sat_cache {
	int resolved;    // -1=not resolved
	int num;
	int component;
};

struct mcg_data {
	struct in6_addr mcg;
	fe_type_t type;
        recv_sec_t sec;
        int vpid;
	struct dvb_frontend_parameters fep;
	int satpos;
	// Small temporary cache for SAT-resolution
	struct sat_cache sat_cache[MAX_TUNER_CACHE];
}; 

void print_fe_info (struct dvb_frontend_info *fe_info);
void print_mcg (struct in6_addr *mcg);
void print_frontend_settings (struct dvb_frontend_parameters *fe_parms);

DLL_SYMBOL void fe_parms_to_mcg (struct in6_addr *mcg, streaming_group_t StreamingGroup, fe_type_t type, recv_sec_t * sec, struct dvb_frontend_parameters *fep, int vpid);
DLL_SYMBOL int mcg_to_fe_parms (struct in6_addr *mcg, fe_type_t * type, recv_sec_t * sec, struct dvb_frontend_parameters *fep, int *vpid);
DLL_SYMBOL int mcg_to_all_parms(struct in6_addr *mcg, struct mcg_data * mcd);

DLL_SYMBOL void mcg_set_streaming_group (struct in6_addr *mcg, streaming_group_t StreamingGroup);
DLL_SYMBOL void mcg_get_streaming_group (struct in6_addr *mcg, streaming_group_t *StreamingGroup);
DLL_SYMBOL void mcg_init_streaming_group (struct in6_addr *mcg, streaming_group_t StreamingGroup);

DLL_SYMBOL void mcg_set_pid (struct in6_addr *mcg, int pid);
DLL_SYMBOL void mcg_get_pid (struct in6_addr *mcg, int *pid);

DLL_SYMBOL void mcg_get_priority (struct in6_addr *mcg, int *priority);
DLL_SYMBOL void mcg_set_priority (struct in6_addr *mcg, int priority);
                                
DLL_SYMBOL void mcg_get_satpos (struct in6_addr *mcg, int *satpos);
DLL_SYMBOL void mcg_set_satpos (struct in6_addr *mcg, int satpos);

DLL_SYMBOL void mcg_get_id (struct in6_addr *mcg, int *id);
DLL_SYMBOL void mcg_set_id (struct in6_addr *mcg, int id);


int gzip (Bytef * dest, unsigned int *destLen, const Bytef * source, unsigned int sourceLen, int level);
int gunzip (Bytef * dest, unsigned int *destLen, const Bytef * source, unsigned int sourceLen);
void print_trace (void);
void SignalHandlerCrash(int signum);

int syslog_init(void);
int syslog_write(char *s);
void syslog_exit(void);

#endif
