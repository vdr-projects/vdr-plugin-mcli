/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#include "headers.h"
#define CI_RESET_WAIT 10

#ifdef __MINGW32__
#include <getopt.h>
#endif

cmdline_t cmd;

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void print_help (int argc, char *argv[])
{
	printf ("Usage:\n" \
		"	mcli --ifname <network interface>\n" \
		"	mcli --port <port> (default: -port 23000)\n" \
		"	mcli --dvb-s <num> --dvb-c <num> --dvb-t <num> --atsc <num> --dvb-s2 <num>\n" \
		"             limit number of device types (default: 8 of every type)\n" \
		"	mcli --diseqc-conf <filepath>\n" \
		"	mcli --rotor-conf <filepath>\n" \
		"	mcli --mld-reporter-disable\n" \
		"	mcli --sock-path <filepath>\n"\
		"	mcli --ca-enable <bitmask>\n"\
		"	mcli --ci-timeout <time>\n"\
		"	mcli --vdr-diseqc-bind <0|1>\n"\
		"	mcli --reel-cam-mode\n"\
		"\n");
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void init_cmd_line_parameters ()
{
	int i;
	memset (&cmd, 0, sizeof (cmdline_t));

	for (i=0; i<=FE_DVBS2; i++) {
		cmd.tuner_type_limit[i] = 8;
	}
	cmd.port = 23000;
	cmd.mld_start = 1;
	cmd.ca_enable = 3;
	cmd.vdrdiseqcmode = 1;
	cmd.reelcammode = 0;
	cmd.ci_timeout = CI_RESET_WAIT;
	strcpy (cmd.cmd_sock_path, API_SOCK_NAMESPACE);
	cmd.disec_conf_path[0]=0;
	cmd.rotor_conf_path[0]=0;
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void get_options (int argc, char *argv[])
{
	int tuners = 0, i;
	char c;
	int ret;
	//init parameters
	init_cmd_line_parameters ();
	while (1) {
		//int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"port", 1, 0, 0},	//0
			{"ifname", 1, 0, 0},	//1
			{"help", 0, 0, 0},	//2
			{"dvb-s", 1, 0, 0},	//3
			{"dvb-c", 1, 0, 0},	//4
			{"dvb-t", 1, 0, 0},	//5
			{"atsc", 1, 0, 0},	//6
			{"dvb-s2", 1, 0, 0},	//7
			{"diseqc-conf", 1, 0, 0},	//8
			{"mld-reporter-disable", 0, 0, 0},	//9
			{"sock-path", 1, 0, 0},	//10
			{"ca-enable", 1, 0, 0},	//11
			{"ci-timeout", 1, 0, 0},	//12
			{"vdr-diseqc-bind", 1, 0, 0},	//13
			{"reel-cam-mode", 0, 0, 0},     //14
			{"rotor-conf", 1, 0, 0},	//15
			{NULL, 0, 0, 0}
		};

		ret = getopt_long_only (argc, argv, "", long_options, &option_index);
		c=(char)ret;
		if (ret == -1 || c == '?') {
			break;
		}

		switch (option_index) {
		case 0:
			cmd.port = atoi (optarg);
			break;
		case 1:
			strncpy (cmd.iface, optarg, IFNAMSIZ-1);
			break;
		case 2:
			print_help (argc, argv);
			exit (0);
			break;
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			i = atoi (optarg);
			if (!tuners) {
				memset (cmd.tuner_type_limit, 0, sizeof (cmd.tuner_type_limit));
			}
			cmd.tuner_type_limit[option_index - 3] = i;
			tuners += i;
			break;
		case 8:
			strncpy (cmd.disec_conf_path, optarg, _POSIX_PATH_MAX-1);
			break;
		case 9:
			cmd.mld_start = 0;
			break;
		case 10:
			strncpy (cmd.cmd_sock_path, optarg, _POSIX_PATH_MAX-1);
			break;
		case 11:
			cmd.ca_enable=atoi(optarg);
			break;
		case 12:
			cmd.ci_timeout=atoi(optarg);
			break;
		case 13:
			cmd.vdrdiseqcmode=atoi(optarg);
			break;
		case 14:
			cmd.reelcammode = 1;
			break;
		case 15:
			strncpy (cmd.rotor_conf_path, optarg, _POSIX_PATH_MAX-1);
			break;
		default:
			printf ("?? getopt returned character code 0%o ??\n", c);
		}
	}
}
