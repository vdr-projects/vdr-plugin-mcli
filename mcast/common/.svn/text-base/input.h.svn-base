/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#ifndef __INPUT_H__
#define __INPUT_H__
typedef struct
{
	int port;
	char iface[IFNAMSIZ];
	time_t start_time;
#ifdef SERVER
	int tuner_number;
	char cfgpath[_POSIX_PATH_MAX];
	int verbose;
#endif
#ifdef CLIENT
	char disec_conf_path[_POSIX_PATH_MAX];
	char rotor_conf_path[_POSIX_PATH_MAX];
	char cmd_sock_path[_POSIX_PATH_MAX];
	int tuner_type_limit[FE_DVBS2+1];
	int mld_start;
	int ca_enable;
	int ci_timeout;
	int vdrdiseqcmode;
	int reelcammode;
#endif		
} cmdline_t;

extern cmdline_t cmd;

void get_options (int argc, char *argv[]);

#endif
