/*------------------------------------------------------------------------
 * netcvdiag - NetCeiver diagnosis tool
 *
 *------------------------------------------------------------------------*/

#include "headers.h"

#ifdef __MINGW32__
#include <getopt.h>
#endif

#ifdef API_SOCK

/*------------------------------------------------------------------------*/
#define API_WAIT_RESPONSE(cmd) { cmd->state=API_REQUEST; \
		send (sock_comm, &sock_cmd, sizeof(api_cmd_t), 0); \
		recv (sock_comm, &sock_cmd, sizeof(api_cmd_t), 0); \
		if (cmd->state == API_ERROR) warn ( "SHM parameter error\n");}

int sock_comm;

int nc_api_init(char *path)
{
        int sock_name_len = 0;
        struct sockaddr_un sock_name;
        sock_name.sun_family = AF_UNIX;

        strcpy(sock_name.sun_path, path);
        sock_name_len = sizeof(struct sockaddr_un);
        
        if((sock_comm = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        {
                warn ("socket create failure %d\n", errno);
                return -1;
        }

        if (connect(sock_comm, (struct sockaddr*)&sock_name, sock_name_len) < 0) {
                warn ("connect failure to %s: %s\n",path, strerror(errno));
		return -1;
        }
	return 0;
}
#endif
#ifdef API_WIN
/*------------------------------------------------------------------------*/
#define API_WAIT_RESPONSE(cmd) { cmd->state=API_REQUEST; \
	WriteFile( hPipe, &sock_cmd, sizeof(api_cmd_t), &cbWritten, NULL); \
	ReadFile(  hPipe, &sock_cmd, sizeof(api_cmd_t), &cbRead, NULL); \
	if (cmd->state == API_ERROR) {warn ( "SHM parameter error, incompatible versions?\n"); exit(-1);}}

HANDLE hPipe;
DWORD cbRead, cbWritten; 

int nc_api_init(char *path)
{
	hPipe = CreateFile( 
         TEXT("\\\\.\\pipe\\mcli"),   // pipe name 
         GENERIC_READ |  // read and write access 
         GENERIC_WRITE, 
         0,              // no sharing 
         NULL,           // default security attributes
         OPEN_EXISTING,  // opens existing pipe 
         0,              // default attributes 
         NULL);          // no template file 

	if (hPipe == INVALID_HANDLE_VALUE) {
		warn("CreatePipe failed"); 
		return -1;
	}
	return 0;
}
#endif
/*------------------------------------------------------------------------*/
void usage(void)
{
	fprintf(stderr,
		"netcvdiag - NetCeiver diagnosis tool,  version " MCLI_VERSION_STR "\n"
		"(c) BayCom GmbH\n"
		"Usage: netcvdiag <options>\n"
		"Options:   -a                    Show all\n"
		"           -u                    Show UUIDs\n"
		"           -t                    Show tuners\n"
		"           -c                    Get NetCeiver count\n"
		"           -S                    Show satellite settings\n"
		"           -s                    Show tuner state\n"
		"           -r <n>                Repeat every n seconds\n"
		"           -v                    Show HW/SW-versions\n"
		"           -P <path>             Set API socket\n"
		);
	exit(0);
}
/*------------------------------------------------------------------------*/
void show_it(int show_count, int show_uuids, int show_tuners, int show_sats, int show_versions, int show_cams)
{
	int nc_num;
	int i;
	time_t now=time(0);
	api_cmd_t sock_cmd;
	api_cmd_t *api_cmd=&sock_cmd;

	api_cmd->cmd=API_GET_NC_NUM;
	api_cmd->magic = MCLI_MAGIC;
	api_cmd->version = MCLI_VERSION;

        API_WAIT_RESPONSE(api_cmd);
	if(api_cmd->magic != MCLI_MAGIC || api_cmd->version != MCLI_VERSION) {
		info("API version mismatch!\n");
		return;
	}
	if (show_count)
		printf("Count: %i\n", api_cmd->parm[API_PARM_NC_NUM]);
	nc_num=api_cmd->parm[API_PARM_NC_NUM];

	for(i=0;i<nc_num;i++) {
                api_cmd->cmd=API_GET_NC_INFO;
                api_cmd->parm[API_PARM_NC_NUM]=i;
                API_WAIT_RESPONSE(api_cmd);
                if(api_cmd->u.nc_info.magic != MCLI_MAGIC || api_cmd->u.nc_info.version != MCLI_VERSION) {
                	err("API version mismatch!\n");
                }
		if (show_uuids||show_versions) {
			char buf[UUID_SIZE];
			if(strlen(api_cmd->u.nc_info.Description)) {
				sprintf(buf, "%s, ", api_cmd->u.nc_info.Description);
			} else {
				buf[0]=0;
			}
			printf("NetCeiver %i:\n"
			       "  UUID <%s>, %s%s, tuners %d\n",
			       i,
			       api_cmd->u.nc_info.uuid, 
			       buf,
			       (unsigned int) api_cmd->u.nc_info.lastseen<(now-10)?"DEAD":"ALIVE",
			       api_cmd->u.nc_info.tuner_num);
		}
		if (show_versions) {
			printf("  OS <%s>, App <%s>, FW <%s>, HW <%s>\n",
			       api_cmd->u.nc_info.OSVersion, api_cmd->u.nc_info.AppVersion,
			       api_cmd->u.nc_info.FirmwareVersion, api_cmd->u.nc_info.HardwareVersion
				);
			printf("  Serial <%s>, Vendor <%s>, state %i\n",
			       api_cmd->u.nc_info.Serial, api_cmd->u.nc_info.Vendor, api_cmd->u.nc_info.DefCon);
			printf("  SystemUptime %d, ProcessUptime %d\n",
			       (int)api_cmd->u.nc_info.SystemUptime, (int)api_cmd->u.nc_info.ProcessUptime);
			printf("  TunerTimeout %d\n",
			       (int)api_cmd->u.nc_info.TunerTimeout);
		}
		if (show_cams) {
			int i;
			for (i = 0;  i < api_cmd->u.nc_info.cam_num; i++) {
				char *camstate="";
				char *cammode="";
				
				switch(api_cmd->u.nc_info.cam[i].status) {
					case DVBCA_CAMSTATE_MISSING: 
						camstate="MISSING"; break;
					case DVBCA_CAMSTATE_INITIALISING:
						camstate="INIT"; break;
					case DVBCA_CAMSTATE_READY:
						camstate="READY"; break;
				}
				switch(api_cmd->u.nc_info.cam[i].flags) {
					case CA_SINGLE:
						cammode="CA_SINGLE";break;
					case CA_MULTI_SID:
						cammode="CA_MULTI_SID";break;
					case CA_MULTI_TRANSPONDER:
						cammode="CA_MULTI_TRANSPONDER";break;
				}	
				printf("  CI-Slot %d: State <%s>, Mode <%s>, CAPMT-Flag: %d, SIDs %d/%d, CAM <%s>\n", api_cmd->u.nc_info.cam[i].slot, camstate, cammode, api_cmd->u.nc_info.cam[i].capmt_flag, api_cmd->u.nc_info.cam[i].use_sids, api_cmd->u.nc_info.cam[i].max_sids, api_cmd->u.nc_info.cam[i].menu_string);
			}
		}
		if (show_tuners) {
			int j;
			int tuner_num=api_cmd->u.nc_info.tuner_num;
			for(j=0;j<tuner_num;j++) {
				api_cmd->cmd=API_GET_TUNER_INFO;
				api_cmd->parm[API_PARM_TUNER_NUM]=j;
				API_WAIT_RESPONSE(api_cmd);
				printf("  Tuner %i: <%s>,  SatList: <%s>, Preference %i\n",
				       j,
				       api_cmd->u.tuner_info.fe_info.name,
				       api_cmd->u.tuner_info.SatelliteListName,
				       api_cmd->u.tuner_info.preference
				       );
			}
			puts("");
		}

		if (show_sats) {
			int sat_list_num=api_cmd->u.nc_info.sat_list_num;
			int j;
			for(j=0;j<sat_list_num;j++) {
				api_cmd->cmd=API_GET_SAT_LIST_INFO;
				api_cmd->parm[API_PARM_NC_NUM]=i;
				api_cmd->parm[API_PARM_SAT_LIST_NUM]=j;
				API_WAIT_RESPONSE(api_cmd);
		                if(api_cmd->u.sat_list.magic != MCLI_MAGIC || api_cmd->u.sat_list.version != MCLI_VERSION) {
                			err("API version mismatch!\n");
		                }
                
				printf("NetCeiver %i: SatList <%s>, entries %d\n",
				       i,
				       api_cmd->u.sat_list.Name,
				       api_cmd->u.sat_list.sat_num);
                
				int sat_num=api_cmd->u.sat_list.sat_num;
				int k;
				for(k=0;k<sat_num;k++) {
					api_cmd->cmd=API_GET_SAT_INFO;
					api_cmd->parm[API_PARM_SAT_LIST_NUM]=j;
					api_cmd->parm[API_PARM_SAT_NUM]=k;
					API_WAIT_RESPONSE(api_cmd);
					int comp_num=api_cmd->u.sat_info.comp_num;
					float pos=(float)((api_cmd->u.sat_info.SatPos-1800.0)/10.0);
					float minr=(float)((api_cmd->u.sat_info.SatPosMin-1800.0)/10.0);
					float maxr=(float)((api_cmd->u.sat_info.SatPosMax-1800.0)/10.0);
					float af=(float)((api_cmd->u.sat_info.AutoFocus)/10.0);
					float longitude=(float)((api_cmd->u.sat_info.Longitude)/10.0);
					float latitude=(float)((api_cmd->u.sat_info.Latitude)/10.0);

					printf("  Satname: <%s>, Position <%.1f%c>, entries %i\n",
					       api_cmd->u.sat_info.Name,
					       fabs(pos),pos<0?'W':'E',
					       comp_num);

					if (api_cmd->u.sat_info.type==SAT_SRC_ROTOR) 
						printf("    Rotor: Range <%.1f%c>-<%.1f%c>, AF <%.1f>, Long <%.1f%c>, Lat <%.1f%c>\n",
						       fabs(minr),minr<0?'W':'E',
						       fabs(maxr),maxr<0?'W':'E',
						       fabs(af),
						       fabs(longitude),longitude<0?'W':'E',
						       fabs(latitude),longitude<0?'S':'N');

					int l;
					for(l=0;l<comp_num;l++) {
						api_cmd->cmd=API_GET_SAT_COMP_INFO;
						api_cmd->parm[API_PARM_SAT_LIST_NUM]=j;
						api_cmd->parm[API_PARM_SAT_NUM]=k;
						api_cmd->parm[API_PARM_SAT_COMP_NUM]=l;
						API_WAIT_RESPONSE(api_cmd);
						int m=0,n;
						char diseqc[256];
						char *ptr=diseqc;
						struct dvb_diseqc_master_cmd *diseqc_cmd=&api_cmd->u.sat_comp.sec.diseqc_cmd;
						
						diseqc[0]=0;
						
						for(n=0;n<api_cmd->u.sat_comp.diseqc_cmd_num;n++) {
							for(*ptr=0,m=0;m<diseqc_cmd->msg_len;m++) {
								ptr+=sprintf(ptr, "%02X ", diseqc_cmd->msg[m]);
							}
							ptr+=sprintf(ptr, ", ");
							diseqc_cmd=api_cmd->u.sat_comp.diseqc_cmd+n;
						}
						if(m>0) {
							*(ptr-3)=0;
						}
						char *mini="MINI_OFF";
						switch(api_cmd->u.sat_comp.sec.mini_cmd) {
							case SEC_MINI_A:mini="MINI_A  ";break;
							case SEC_MINI_B:mini="MINI_B  ";break;
						}
						printf("    Entry %i: Polarisation %c, Min% 6d, "
						       "Max% 6d, LOF% 6d %s %s %s DiSEqC <%s>\n",
						       l,
						       api_cmd->u.sat_comp.Polarisation?'H':'V',
						       api_cmd->u.sat_comp.RangeMin,
						       api_cmd->u.sat_comp.RangeMax,
						       api_cmd->u.sat_comp.LOF,
						       mini,
						       api_cmd->u.sat_comp.sec.tone_mode==SEC_TONE_ON ?"TONE_ON ":"TONE_OFF",
						       api_cmd->u.sat_comp.sec.voltage==SEC_VOLTAGE_18?"VOLTAGE_18":"VOLTAGE_13",
						       diseqc
						       );
					}
				}
			}
		}
	}
	puts("");
}
/*------------------------------------------------------------------------*/
void show_stats(void)
{
	api_cmd_t sock_cmd;
        api_cmd_t *api_cmd=&sock_cmd;
	int i;
	char *types[]={"DVB-S","DVB-C","DVB-T", "?", "DVB-S2"};
	int type;

	api_cmd->cmd=API_GET_TRA_NUM;
	api_cmd->magic = MCLI_MAGIC;
	api_cmd->version = MCLI_VERSION;
	API_WAIT_RESPONSE(api_cmd);
        
//	printf("tra_num: %d\n", api_cmd->parm[API_PARM_TRA_NUM]);
	int tra_num=api_cmd->parm[API_PARM_TRA_NUM];
	for(i=0;i<tra_num;i++) {
		char uuid[256];
		char *p;

		api_cmd->cmd=API_GET_TRA_INFO;
		api_cmd->parm[API_PARM_TRA_NUM]=i;
		API_WAIT_RESPONSE(api_cmd);
		char host[INET6_ADDRSTRLEN];
		inet_ntop (AF_INET6, &api_cmd->u.tra.mcg, (char *) host, INET6_ADDRSTRLEN);
		type=api_cmd->u.tra.fe_type;
		if (type<0 || type>4)
			type=3;

		strncpy(uuid,api_cmd->u.tra.uuid,255);
		uuid[255]=0;
		p=strrchr(uuid,':');
		if (p)
			*p=0;
		
		fe_type_t t; 
		recv_sec_t sec; 
		int satpos;
		struct dvb_frontend_parameters fep;
		
		if(mcg_to_fe_parms(&api_cmd->u.tra.mcg, &t, &sec, &fep, NULL)<0) {
			memset(&fep,0,sizeof(struct dvb_frontend_parameters));
		}
		
		mcg_get_satpos(&api_cmd->u.tra.mcg, &satpos);
		float pos=(float)((satpos-1800.0)/10.0);
		char pos_str[256];
		if(satpos != 0xfff) {
			sprintf(pos_str, ", position <%.1f%c>", fabs(pos), pos<0?'W':'E');
		} else {
			pos_str[0]=0;
		}
		
		printf("UUID <%s>:\n"
		       "      slot %s%d.%d, type %s, used: % 3d\n"
		       "      %s, frequency %d%s (%.1f%s)%s\n"
		       "      strength %04x, snr %04x, ber %04x, unc %04x\n"
		       "      NIMCurrent %d\n"
		       "      RotorStatus %i, RotorDiff %.1f\n",
		       uuid, 
		       (time(0)-api_cmd->u.tra.lastseen)>15?"-":"",
		       api_cmd->u.tra.slot/2,api_cmd->u.tra.slot%2,
		       types[type], api_cmd->u.tra.InUse, 		       
		       api_cmd->u.tra.s.st==0x1f?"LOCK   ":"NO LOCK", 
		       fep.frequency, (type==1||type==2)?"Hz":"kHz", api_cmd->u.tra.fep.frequency/1000.0, 
		       (type==1||type==2)?"kHz":"MHz",pos_str,
		       api_cmd->u.tra.s.strength, 
		       api_cmd->u.tra.s.snr, api_cmd->u.tra.s.ber,  api_cmd->u.tra.s.ucblocks,  
		       api_cmd->u.tra.NIMCurrent,
		       api_cmd->u.tra.rotor_status,
		       api_cmd->u.tra.rotor_diff/10.0
			);
		
	}
}
/*------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
	int repeat=0;
	int show_uuids=0,show_tuners=0,show_sats=0,show_state=0,show_cams=0;
	int show_count=0, show_versions=0;
#ifdef API_SOCK
	char path[256]=API_SOCK_NAMESPACE;
#endif
#ifdef API_WIN
	char path[256]="\\\\.\\pipe\\mcli";
#endif
	while(1) {
		int ret = getopt(argc,argv, "aucCtsSvr:P:");
		if (ret==-1)
			break;
			
		char c=(char)ret;

		switch (c) {			
		case 'a':
			show_uuids=1;
			show_tuners=1;
			show_sats=1;
			show_state=1;
			show_count=1;
			show_versions=1;
			show_cams=1;
			break;
		case 'u':
			show_uuids=1;
			break;
		case 'c':
			show_count=1;
			break;
		case 'C':
			show_cams=1;
			break;
		case 't':
			show_tuners=1;
			break;
		case 's':
			show_state=1;
			break;
		case 'r':
			repeat=abs(atoi(optarg));
			break;
		case 'S':
			show_sats=1;
			break;
		case 'v':
			show_versions=1;
			break;
		case 'P':
			strncpy(path,optarg,255);
			path[255]=0;
			break;
		default:
			usage();
			break;
		}
	}
	if (nc_api_init(path)==-1) {
		exit(-1);
	}
	
	do {
		show_it(show_count, show_uuids, show_tuners, show_sats, show_versions, show_cams);
		if (show_state)
			show_stats();
		sleep(repeat);
	} while(repeat);

	exit(0);
}

