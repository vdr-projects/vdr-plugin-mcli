/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#include "headers.h"
#if ! (defined WIN32 || defined APPLE)
  #include "dvblo_ioctl.h"
  #include "dvblo_handler.h"
#else
  #include "dummy_client.h"
#endif

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int main (int argc, char **argv)
{
	printf ("DVB - TV Client Version " MCLI_VERSION_STR " (c) BayCom GmbH\n\n");
//#if (defined WIN32 || defined APPLE)
#ifdef WIN32
#ifndef __MINGW32__
	cmdline_t cmd;
	cmd.iface[0]=0;
	cmd.port=0;
	cmd.mld_start=1;
#else
	get_options (argc, argv);
#endif
#else
#ifdef BACKTRACE
	signal(SIGSEGV, SignalHandlerCrash);
	signal(SIGBUS, SignalHandlerCrash);
	signal(SIGABRT, SignalHandlerCrash);
#endif
	get_options (argc, argv);
#endif	
	recv_init (cmd.iface, cmd.port);

	#ifdef API_SHM
		api_shm_init();
	#endif
	#ifdef API_SOCK
		api_sock_init(cmd.cmd_sock_path);
	#endif
	#ifdef API_WIN
		api_init(TEXT("\\\\.\\pipe\\mcli"));
	#endif

	if(cmd.mld_start) {
		mld_client_init (cmd.iface);
	}
#if ! (defined WIN32 || defined APPLE)
	ci_init(cmd.ca_enable, cmd.iface, cmd.port);
	dvblo_init();
	
	dvblo_handler();

	dvblo_exit();
	ci_exit();
#else
	dummy_client ();
#endif

	if(cmd.mld_start) {
		mld_client_exit ();
	}

	#ifdef API_SHM
		api_shm_exit();
	#endif
	#ifdef API_SOCK
		api_sock_exit();
	#endif
	#ifdef API_WIN
		api_exit();
	#endif
	
	recv_exit ();

	return 0;
}
