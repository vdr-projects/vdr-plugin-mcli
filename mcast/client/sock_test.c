/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#include "headers.h"

#define SHM_WAIT_RESPONSE(cmd) { cmd->state=SHM_REQUEST; send (sock_comm, &sock_cmd, sizeof(shm_cmd_t), 0); recv (sock_comm, &sock_cmd, sizeof(shm_cmd_t), 0); if (cmd->state == SHM_ERROR) warn ("SHM parameter error\n");}

int main (int argc, char **argv)
{
	int sock_comm;
	int sock_name_len = 0;
	struct sockaddr sock_name;
	shm_cmd_t sock_cmd;
	shm_cmd_t *shm_cmd=&sock_cmd;
	sock_name.sa_family = AF_UNIX;

	strcpy(sock_name.sa_data, SOCK_NAMESPACE);
	sock_name_len = strlen(sock_name.sa_data) + sizeof(sock_name.sa_family);
	
	if((sock_comm = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
        	warn ("socket create failure %d\n", errno);
		return -1;
	}

	if (connect(sock_comm, &sock_name, sock_name_len) < 0) {
		err ("connect failure\n");
	}
				
	shm_cmd->cmd=SHM_GET_NC_NUM;
	SHM_WAIT_RESPONSE(shm_cmd);
	
	printf("nc_num: %d\n", shm_cmd->parm[SHM_PARM_NC_NUM]);
	int nc_num=shm_cmd->parm[SHM_PARM_NC_NUM];
	int i;
	for(i=0;i<nc_num;i++) {
		shm_cmd->cmd=SHM_GET_NC_INFO;
		shm_cmd->parm[SHM_PARM_NC_NUM]=i;
		SHM_WAIT_RESPONSE(shm_cmd);
		printf("nc_info.uuid: %s nc_info.lastseen: %u nc_info.tuner_num: %d\n", shm_cmd->u.nc_info.uuid, (unsigned int) shm_cmd->u.nc_info.lastseen, shm_cmd->u.nc_info.tuner_num);
		int j;
		int tuner_num=shm_cmd->u.nc_info.tuner_num;
		for(j=0;j<tuner_num;j++) {
			shm_cmd->cmd=SHM_GET_TUNER_INFO;
			shm_cmd->parm[SHM_PARM_TUNER_NUM]=j;
			SHM_WAIT_RESPONSE(shm_cmd);
			printf("tuner_info.fe_info.name: %s\n",shm_cmd->u.tuner_info.fe_info.name);
		}
		
		
		int sat_list_num=shm_cmd->u.nc_info.sat_list_num;
		for(j=0;j<sat_list_num;j++) {
			shm_cmd->cmd=SHM_GET_SAT_LIST_INFO;
			shm_cmd->parm[SHM_PARM_NC_NUM]=i;
			shm_cmd->parm[SHM_PARM_SAT_LIST_NUM]=j;
			SHM_WAIT_RESPONSE(shm_cmd);
		
			printf("sat_list_info.Name: %s sat_list_info.sat_num: %d\n", shm_cmd->u.sat_list.Name, shm_cmd->u.sat_list.sat_num);
		
			int sat_num=shm_cmd->u.sat_list.sat_num;
			int k;
			for(k=0;k<sat_num;k++) {
				shm_cmd->cmd=SHM_GET_SAT_INFO;
				shm_cmd->parm[SHM_PARM_SAT_LIST_NUM]=j;
				shm_cmd->parm[SHM_PARM_SAT_NUM]=k;
				SHM_WAIT_RESPONSE(shm_cmd);
				printf("sat_info.Name: %s\n",shm_cmd->u.sat_info.Name);
			}
		}
	}

	while (1) {
		shm_cmd->cmd=SHM_GET_TRA_NUM;
		SHM_WAIT_RESPONSE(shm_cmd);
	
		printf("tra_num: %d\n", shm_cmd->parm[SHM_PARM_TRA_NUM]);
		int tra_num=shm_cmd->parm[SHM_PARM_TRA_NUM];
		for(i=0;i<tra_num;i++) {
			shm_cmd->cmd=SHM_GET_TRA_INFO;
			shm_cmd->parm[SHM_PARM_TRA_NUM]=i;
			SHM_WAIT_RESPONSE(shm_cmd);
			printf("tra uuid: %s lastseen: %u lock:%d str:%d snr:%d ber:%d\n", shm_cmd->u.tra.uuid, (unsigned int) shm_cmd->u.tra.lastseen, shm_cmd->u.tra.s.st, shm_cmd->u.tra.s.strength, shm_cmd->u.tra.s.snr, shm_cmd->u.tra.s.ber);
		}
		sleep(2);
	}
	return 0;
}

