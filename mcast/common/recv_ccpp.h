/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#ifndef __RECV_CCPP_H
#define __RECV_CCPP_H

#define XML_BUFLEN 65536
#define TEN_TIMEOUT 2
#define MAX_MENU_STR_LEN 64
#define MAX_CAMS 2


typedef struct tuner_info
{
	int magic;
	int version;

	struct dvb_frontend_info fe_info;
	int slot;
	int preference;
	char uuid[UUID_SIZE];
	char SatelliteListName[UUID_SIZE];
} tuner_info_t;

typedef enum { CA_SINGLE, CA_MULTI_SID, CA_MULTI_TRANSPONDER} nc_ca_caps_t;
enum { DVBCA_CAMSTATE_MISSING, DVBCA_CAMSTATE_INITIALISING, DVBCA_CAMSTATE_READY};

typedef struct cam_info {
	
	uint8_t slot;
	uint8_t status;
	int max_sids;
	int use_sids;
	int capmt_flag;
	int reserved;
	nc_ca_caps_t flags;
	
	char menu_string[MAX_MENU_STR_LEN];

} cam_info_t;

typedef struct netceiver_info
{
	int magic;
	int version;
	
	char OSVersion[UUID_SIZE];
	char AppVersion[UUID_SIZE];
	char FirmwareVersion[UUID_SIZE];
	char HardwareVersion[UUID_SIZE];
	char Serial[UUID_SIZE];
	char Vendor[UUID_SIZE];
	char uuid[UUID_SIZE];
	char Description[UUID_SIZE];
	int TunerTimeout;
	struct in6_addr ip;
	int DefCon;
	time_t SystemUptime;
	time_t ProcessUptime;

	time_t lastseen;

	tuner_info_t *tuner;
	recv_cacaps_t ci;
	satellite_list_t *sat_list;
	cam_info_t cam[MAX_CAMS];

	
	int tuner_num;
	int sat_list_num;
	int cam_num;
} netceiver_info_t;

typedef struct tra
{
	int magic;
	int version;

	recv_festatus_t s;
	fe_type_t fe_type;
	struct dvb_frontend_parameters fep;
	struct in6_addr mcg;
	int slot;
	char uuid[UUID_SIZE];
	int redirect;
	int NIMCurrent;
	int InUse;
	int rotor_status;	
	time_t lastseen;
	int rotor_diff;
#ifdef P2P
	int preference;
	int token;
#endif

} tra_t;

typedef struct tra_info
{
	int magic;
	int version;

	tra_t *tra;
	int tra_num;
	cam_info_t cam[MAX_CAMS];
	int cam_num;
#ifdef P2P
	int quit;	
	int tca_id;
	int mca_grps;
	struct in6_addr ipv6;
#endif

} tra_info_t;

typedef struct recv_info recv_info_t;

void *recv_ten (void *arg);
void *recv_tca (void *arg);
void *recv_tra (void *arg);
int get_tca_data (xmlChar * xmlbuff, int buffersize, netceiver_info_t * nc_info);
int get_tra_data (xmlChar * xmlbuff, int buffersize, tra_info_t * tra_info);
DLL_SYMBOL int register_ten_handler (recv_info_t * r, int (*p)(tra_t *, void *), void *c);
#endif
