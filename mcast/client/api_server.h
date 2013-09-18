/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#define API_SHM_NAMESPACE "/mcli"
#define API_SOCK_NAMESPACE "/var/tmp/mcli.sock"

typedef enum { 	API_IDLE,
		API_REQUEST,
		API_RESPONSE,
		API_ERROR
} api_state_t;

typedef enum { 	API_GET_NC_NUM,
		API_GET_NC_INFO,
		API_GET_TUNER_INFO,
		API_GET_SAT_LIST_INFO,
		API_GET_SAT_INFO,
		API_GET_SAT_COMP_INFO,
		API_GET_TRA_NUM,
		API_GET_TRA_INFO,
		API_GET_DEVICE_INFO
} api_cmdval_t;

typedef enum {	API_PARM_NC_NUM=0,
		API_PARM_DEVICE_NUM=0,
		API_PARM_TUNER_NUM,
		API_PARM_SAT_LIST_NUM,
		API_PARM_SAT_NUM,
		API_PARM_SAT_COMP_NUM,
		API_PARM_TRA_NUM,
		API_PARM_MAX
} api_parm_t;

typedef struct {
	int magic;
	int version;

	api_cmdval_t cmd;
	api_state_t state;
	int parm[API_PARM_MAX];
	union {
		netceiver_info_t nc_info;
		tuner_info_t tuner_info;
		satellite_list_t sat_list;
		satellite_info_t sat_info;
		satellite_component_t sat_comp;
		tra_t tra;
	} u;
} api_cmd_t;

#ifdef API_SHM
DLL_SYMBOL int api_shm_init (void);
DLL_SYMBOL void api_shm_exit (void);
#endif
#ifdef API_SOCK
DLL_SYMBOL int api_sock_init (const char *cmd_sock_path);
DLL_SYMBOL void api_sock_exit (void);
#endif
#ifdef API_WIN
DLL_SYMBOL int api_init (LPTSTR cmd_pipe_path);
DLL_SYMBOL void api_exit (void);
#endif
