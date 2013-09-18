/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#define DEBUG 1
#include "headers.h"


//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static int dummy_handle_ts (unsigned char *buffer, size_t len, void *p)
{
	FILE *f=(FILE*)p;
	fwrite(buffer, len, 1, f);
	return len;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static int dummy_handle_ten (tra_t *ten, void *p)
{
	FILE *f=(FILE*)p;
	if(ten) {
		fprintf(f,"Status: %02X, Strength: %04X, SNR: %04X, BER: %04X\n",ten->s.st,ten->s.strength, ten->s.snr, ten->s.ber);
	}
	return 0;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void dummy_client (void)
{
	int i;
	int n;
	int run=1;
	FILE *f;
	recv_info_t *r;
	recv_sec_t sec;
	struct dvb_frontend_parameters fep;
	dvb_pid_t pids[3];

	netceiver_info_list_t *nc_list=nc_get_list();
#if 0
	printf("Looking for netceivers out there....\n");
	while(run) {
		nc_lock_list();
		for (n = 0; n < nc_list->nci_num; n++) {
			netceiver_info_t *nci = nc_list->nci + n;
			printf("\nFound NetCeiver: %s\n",nci->uuid);
			for (i = 0; i < nci->tuner_num; i++) {
				printf("  Tuner: %s, Type %d\n",nci->tuner[i].fe_info.name,  nci->tuner[i].fe_info.type);
			}
		}
		nc_unlock_list();
		if(nc_list->nci_num) {
			break;
		}
		sleep(1);
	}
#endif
	f=fopen("out.ts","wb");

	r = recv_add();
	if (!r) {
		fprintf (stderr, "Cannot get memory for receiver\n");
		return;
	}
	register_ten_handler (r, dummy_handle_ten, stderr);
	register_ts_handler (r, dummy_handle_ts, f);

	memset(&sec, 0, sizeof(recv_sec_t));
	sec.voltage=SEC_VOLTAGE_18;
	sec.mini_cmd=SEC_MINI_A;
	sec.tone_mode=SEC_TONE_ON;
	
	memset(&fep, 0, sizeof(struct dvb_frontend_parameters));
	fep.frequency=12544000;
	fep.inversion=INVERSION_AUTO;
	fep.u.qpsk.symbol_rate=22000000;
	fep.u.qpsk.fec_inner=FEC_5_6;
	
	memset(&pids, 0, sizeof(pids));
	pids[0].pid=511;
	pids[1].pid=512;
	pids[2].pid=511;
	pids[2].id=2;
	pids[3].pid=511;
	pids[3].id=1;
	pids[4].pid=-1;
	
	printf("\nTuning a station and writing transport data to file 'out.ts':\n");
	recv_tune (r, (fe_type_t)FE_QPSK, 1800+192, &sec, &fep, pids);
	getchar();
	register_ten_handler (r, NULL, NULL);
	register_ts_handler (r, NULL, NULL);
	fclose(f);
}
