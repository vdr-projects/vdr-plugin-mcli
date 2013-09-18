/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#include "headers.h"


int main (int argc, char **argv)
{
#ifdef USE_SHM_API
	#define API_WAIT_RESPONSE(cmd) { cmd->state=API_REQUEST; while (cmd->state == API_REQUEST) usleep(10*1000); if (cmd->state == API_ERROR) warn ("SHM parameter error\n");}
	
	int fd = shm_open (API_SHM_NAMESPACE, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if (fd == -1 ) {
		warn ("Cannot get a shared memory handle\n");
		return -1;
	}

	if (ftruncate (fd, sizeof(api_cmd_t)) == -1) {
		err ("Cannot truncate shared memory\n");
	}
	
	api_cmd_t *api_cmd = mmap(NULL, sizeof(api_cmd_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if ( api_cmd == MAP_FAILED ) {
		err ("MMap of shared memory region failed\n");
	}
	close(fd);
#endif
#ifdef USE_SOCK_API
	#define API_WAIT_RESPONSE(cmd) { cmd->state=API_REQUEST; send (sock_comm, &sock_cmd, sizeof(api_cmd_t), 0); recv (sock_comm, &sock_cmd, sizeof(api_cmd_t), 0); if (cmd->state == API_ERROR) warn ("SHM parameter error\n");}

	int sock_comm;
	int sock_name_len = 0;
	struct sockaddr_un sock_name;
	api_cmd_t sock_cmd;
	api_cmd_t *api_cmd=&sock_cmd;
	sock_name.sun_family = AF_UNIX;

	strcpy(sock_name.sun_path, API_SOCK_NAMESPACE);
	sock_name_len = strlen(API_SOCK_NAMESPACE) + sizeof(sock_name.sun_family);
	
	if((sock_comm = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
        	warn ("socket create failure %d\n", errno);
		return -1;
	}

	if (connect(sock_comm, &sock_name, sock_name_len) < 0) {
		err ("connect failure\n");
	}
#endif	
	api_cmd->cmd=API_GET_NC_NUM;
	API_WAIT_RESPONSE(api_cmd);
	
	printf("nc_num: %d\n", api_cmd->parm[API_PARM_NC_NUM]);
	int nc_num=api_cmd->parm[API_PARM_NC_NUM];
	int i;
	for(i=0;i<nc_num;i++) {
		api_cmd->cmd=API_GET_NC_INFO;
		api_cmd->parm[API_PARM_NC_NUM]=i;
		API_WAIT_RESPONSE(api_cmd);
		printf("nc_info.uuid: %s nc_info.lastseen: %u nc_info.tuner_num: %d\n", api_cmd->u.nc_info.uuid, (unsigned int) api_cmd->u.nc_info.lastseen, api_cmd->u.nc_info.tuner_num);
		int j;
		int tuner_num=api_cmd->u.nc_info.tuner_num;
		for(j=0;j<tuner_num;j++) {
			api_cmd->cmd=API_GET_TUNER_INFO;
			api_cmd->parm[API_PARM_TUNER_NUM]=j;
			API_WAIT_RESPONSE(api_cmd);
			printf("tuner_info.fe_info.name: %s SatList: %s\n",api_cmd->u.tuner_info.fe_info.name, api_cmd->u.tuner_info.SatelliteListName);
		}
		
		
		int sat_list_num=api_cmd->u.nc_info.sat_list_num;
		for(j=0;j<sat_list_num;j++) {
			api_cmd->cmd=API_GET_SAT_LIST_INFO;
			api_cmd->parm[API_PARM_NC_NUM]=i;
			api_cmd->parm[API_PARM_SAT_LIST_NUM]=j;
			API_WAIT_RESPONSE(api_cmd);
		
			printf("sat_list_info.Name: %s sat_list_info.sat_num: %d\n", api_cmd->u.sat_list.Name, api_cmd->u.sat_list.sat_num);
		
			int sat_num=api_cmd->u.sat_list.sat_num;
			int k;
			for(k=0;k<sat_num;k++) {
				api_cmd->cmd=API_GET_SAT_INFO;
				api_cmd->parm[API_PARM_SAT_LIST_NUM]=j;
				api_cmd->parm[API_PARM_SAT_NUM]=k;
				API_WAIT_RESPONSE(api_cmd);
				printf("sat_info.Name: %s\n",api_cmd->u.sat_info.Name);
				int comp_num=api_cmd->u.sat_info.comp_num;
				int l;
				for(l=0;l<comp_num;l++) {
					api_cmd->cmd=API_GET_SAT_COMP_INFO;
					api_cmd->parm[API_PARM_SAT_LIST_NUM]=j;
					api_cmd->parm[API_PARM_SAT_NUM]=k;
					api_cmd->parm[API_PARM_SAT_COMP_NUM]=l;
					API_WAIT_RESPONSE(api_cmd);
					printf("sat_comp.Polarisation: %d sat_comp.RangeMin: %d sat_comp.RangeMax: %d sat_comp.LOF: %d\n", api_cmd->u.sat_comp.Polarisation, api_cmd->u.sat_comp.RangeMin, api_cmd->u.sat_comp.RangeMax, api_cmd->u.sat_comp.LOF);
				}
			}
		}
	}

	while (1) {
		api_cmd->cmd=API_GET_TRA_NUM;
		API_WAIT_RESPONSE(api_cmd);
	
		printf("tra_num: %d\n", api_cmd->parm[API_PARM_TRA_NUM]);
		int tra_num=api_cmd->parm[API_PARM_TRA_NUM];
		for(i=0;i<tra_num;i++) {
			api_cmd->cmd=API_GET_TRA_INFO;
			api_cmd->parm[API_PARM_TRA_NUM]=i;
			API_WAIT_RESPONSE(api_cmd);
			char host[INET6_ADDRSTRLEN];
			inet_ntop (AF_INET6, &api_cmd->u.tra.mcg, (char *) host, INET6_ADDRSTRLEN);
                        
			printf("tra.slot:%d tra.fe_type: %d tra.InUse: % 3d tra.mcg: %s tra.uuid: %s tra.lastseen: %u tra.lock:%d tra.strength:%d tra.snr:%d tra.ber:%d\n", api_cmd->u.tra.slot, api_cmd->u.tra.fe_type, api_cmd->u.tra.InUse, host, api_cmd->u.tra.uuid, (unsigned int) api_cmd->u.tra.lastseen, api_cmd->u.tra.s.st, api_cmd->u.tra.s.strength, api_cmd->u.tra.s.snr, api_cmd->u.tra.s.ber);
		}
		sleep(2);
	}
	return 0;
}

