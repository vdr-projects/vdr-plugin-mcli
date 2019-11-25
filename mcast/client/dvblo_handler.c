/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#include "headers.h"
#include "dvblo_ioctl.h"
#include "dvblo_handler.h"

//#define SHOW_EVENTS
//#define SHOW_TPDU
//#define SHOW_PIDS

#define TUNE_FORCED_TIMEOUT 5

extern pthread_mutex_t lock;
extern recv_info_t receivers;

static dvblo_dev_t devs;
static int dev_num = 0;
static int dvblo_run = 1;
static int cidev = 0;
static int reload = 0;
static dvblo_cacaps_t cacaps;

static int special_status_mode=0;   // 1: return rotor mode and tuner slot in status

static int dvblo_get_nc_addr (char *addrstr, char *uuid, const int addrstr_len)
{
	int len = strlen (uuid);
	if (len <= 5) {
		return -1;
	};
	memset (addrstr, 0, addrstr_len);

	strncpy (addrstr, uuid, addrstr_len);

	// avoid -Wstringop-overflow
	int len_short = len - 5;
	if (len_short > addrstr_len -1) {
		len_short = addrstr_len -1;
	};
	addrstr[len_short] = '\0';
	return 0;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static int dvblo_handle_ts (unsigned char *buffer, size_t len, void *p)
{
	 dvblo_dev_t * d=( dvblo_dev_t *)p;
	return write (d->fd, buffer, len);
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static int dvblo_handle_ten (tra_t * ten, void *c)
{
	dvblo_dev_t *d = (dvblo_dev_t *) c;
	dbg ("TEN: %ld %p %p\n", gettid (), ten, d);

	if (ten) {
		dvblo_festatus_t stat;
		memcpy(&stat, &ten->s, sizeof(dvblo_festatus_t));
		
		d->ten = *ten;

		if (special_status_mode) {
			stat.st|=(ten->rotor_status&3)<<8;
			stat.st|=(1+ten->slot)<<12;
		}

		return ioctl (d->fd, DVBLO_SET_FRONTEND_STATUS, (dvblo_festatus_t*)&stat);
	} else {
		dvblo_festatus_t s;
		memset (&s, 0, sizeof (dvblo_festatus_t));
		
		return ioctl (d->fd, DVBLO_SET_FRONTEND_STATUS, &s);
	}
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void sig_handler (int signal)
{
	dbg ("Signal: %d\n", signal);

	switch (signal) {
	case SIGHUP:
		reload = 1;
		break;
	case SIGTERM:
	case SIGINT:
		dbg ("Trying to exit, got signal %d...\n", signal);
		dvblo_run = 0;
		break;
	}
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int dvblo_init (void)
{
	char char_dev[100];
	int j, k = 0;
	struct dvb_frontend_info fe_info;
	dvblo_festatus_t fe_status;

	dvbmc_list_init (&devs.list);

	memset (&fe_info, 0, sizeof (struct dvb_frontend_info));
	fe_info.type = -1;
	strcpy (fe_info.name, "Unconfigured");
	memset (&fe_status, 0, sizeof (dvblo_festatus_t));

	for (j = 0; j < MAX_DEVICES; j++) {

		sprintf (char_dev, "/dev/dvblo%d", j);
		int fd = open (char_dev, O_RDWR);

		if (fd == -1) {
			warn ("Cannot Open %s\n", char_dev);
			continue;
		}
		k++;
		ioctl (fd, DVBLO_SET_FRONTEND_INFO, &fe_info);
		ioctl (fd, DVBLO_SET_FRONTEND_STATUS, &fe_status);
		ioctl (fd, DVBLO_SET_CA_CAPS, &cacaps);
		dvblo_tpdu_t tpdu;
		while (1) {
			if (ioctl (fd, DVBLO_GET_TPDU, &tpdu) || !tpdu.len) {
				break;
			}
		}
		close (fd);
	}

	signal (SIGTERM, &sig_handler);
	signal (SIGINT, &sig_handler);
	signal (SIGHUP, &sig_handler);
	signal (SIGPIPE, &sig_handler);
	return k;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static int dvblo_convert_pids (dvblo_dev_t * d)
{
	dvb_pid_t *dst = d->dstpids;
	dvblo_pids_t *src = &d->pids;
	int i;
#ifdef SHOW_PIDS
	info ("devnum:%d pidnum:%d\n", d->device, src->num);
#endif
	for (i = 0; i < src->num; i++) {
#ifdef SHOW_PIDS
		printf ("devnum:%d pid:%04x\n", d->device, src->pid[i]);
#endif
		dst[i].pid = src->pid[i];
		if (d->ca_enable) {
			dst[i].id = ci_cpl_find_caid_by_pid (src->pid[i]);
			if (dst[i].id != 0) {
				dbg ("pid: %04x id: %04x\n", dst[i].pid, dst[i].id);
			}
		}
	}
	dst[i].pid = -1;
	return i;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static int dvblo_tune (dvblo_dev_t * d, int force)
{
	satellite_reference_t satref;
	int SatPos;
	int mode;
	int ret = 0;

	nc_lock_list ();

	for (mode = 0; mode < 3; mode++) {
		if (satellite_find_by_diseqc (&satref, (recv_sec_t*)&d->sec, &d->fe_parms, mode))
			break;
	}

	if (mode == 3 || (mode == 2 && (d->type == FE_QAM || d->type == FE_OFDM))) {
		SatPos = NO_SAT_POS;
	} else {
		int LOF = 0;		
		if (mode) {
			LOF = satellite_get_lof_by_ref (&satref);
			d->fe_parms.frequency += LOF * 1000;
		}
		SatPos = satellite_get_pos_by_ref (&satref);
		recv_sec_t *sec=satellite_find_sec_by_ref (&satref);
		memcpy(&d->sec, sec, sizeof(recv_sec_t));
		d->sec.voltage = satellite_find_pol_by_ref (&satref);
#if 1 //def SHOW_EVENTS
		printf ("Found satellite position: %d fe_parms: %d LOF: %d voltage: %d mode: %d\n", SatPos, d->fe_parms.frequency, LOF, d->sec.voltage, mode);
#endif
	}
	nc_unlock_list ();
	if (force && d->pids.num == 0) {
		d->dstpids[0].pid = 0;
		d->dstpids[0].id = 0;
		d->dstpids[1].pid = -1;
		ret = 2;
	} else {
		dvblo_convert_pids (d);
	}
	recv_tune (d->r, d->type, SatPos, (recv_sec_t*)&d->sec, &d->fe_parms, d->dstpids);
	return ret;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int dvblo_write_ci (ci_pdu_t * pdu, void *context)
{
	dvblo_dev_t *d = (dvblo_dev_t *) context;
	dvblo_tpdu_t tpdu;
	memcpy (tpdu.data, pdu->data, pdu->len);
	tpdu.len = pdu->len;
	if (!cmd.reelcammode)
		tpdu.data[0] = 0;
	return ioctl (d->fd, DVBLO_SET_TPDU, &tpdu);
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void clean_dvblo_recv_thread (void *argp)
{
	dvblo_dev_t *d = (dvblo_dev_t *) argp;
	recv_del (d->r);
	close (d->fd);
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void *dvblo_recv (void *argp)
{
	dvblo_dev_t *d = (dvblo_dev_t *) argp;
	dvblo_tpdu_t tpdu;
	struct dvb_frontend_parameters old_fe_parms;
	dvblo_sec_t old_sec;
	struct pollfd fds[1];
	int timeout_msecs = 50;
	int ret;
	time_t last_ci_reset = 0;
	char char_dev[100];
	int need_tune = 0;
	time_t forced = 0;
	unsigned int event = 0;

	dvblo_cacaps_t cacap;

	sprintf (char_dev, "/dev/dvblo%d", d->device);
	dbg ("Using character device %s for dvb loopback\n", char_dev);

	d->fd = open (char_dev, O_RDWR);

	if (d->fd < 0) {
		err ("Cannot open %s - dvbloop driver loaded?\n", char_dev);
	}

	pthread_cleanup_push (clean_dvblo_recv_thread, d);

	fds[0].fd = d->fd;

	if (!ioctl (d->fd, DVBLO_IOCCHECKDEV)) {
		if (!ioctl (d->fd, DVBLO_IOCADDDEV)) {
			info ("created dvb adapter: %d\n", d->device);
		}
	}
	d->info.frequency_min = 1;
	d->info.frequency_max = 2000000000;

	ioctl (d->fd, DVBLO_SET_FRONTEND_INFO, &d->info);
	ioctl (d->fd, DVBLO_GET_FRONTEND_PARAMETERS, &d->fe_parms);
	ioctl (d->fd, DVBLO_GET_PIDLIST, &d->pids);
	ioctl (d->fd, DVBLO_GET_SEC_PARAMETERS, &d->sec);

	old_fe_parms = d->fe_parms;
	old_sec = d->sec;
#ifdef DEBUG
	print_fe_info (&d->info);
#endif
	d->recv_run = 1;

	if (dvblo_tune (d, 1) == 2) {
		forced = time (NULL);
	}

	while (d->recv_run) {
		if (d->cacaps->cap.slot_num) {
			nc_lock_list ();
#ifdef SHOW_TPDU
			info ("ca_caps->: %p ci_slot:%d info[0]:%02x info[1]:%02x\n", d->cacaps, d->ci_slot, d->cacaps->info[0].flags, d->cacaps->info[1].flags);
#endif
			cacap = *d->cacaps;
			if (!cmd.reelcammode) {
				if (d->ci_slot != 0) {
					cacap.info[0] = cacap.info[d->ci_slot];
				}
				cacap.cap.slot_num = 1;
			}
			if ((time (NULL) - last_ci_reset) < cmd.ci_timeout) {
				cacap.info[0].flags = 0;
			}
			ioctl (d->fd, DVBLO_SET_CA_CAPS, &cacap);
			nc_unlock_list ();
		}

		fds[0].events = POLLIN;
		ret = poll (fds, 1, timeout_msecs);
		if (ret > 0) {
			ioctl (d->fd, DVBLO_GET_EVENT_MASK, &event);
			ioctl (d->fd, DVBLO_GET_FRONTEND_PARAMETERS, &d->fe_parms);
			ioctl (d->fd, DVBLO_GET_PIDLIST, &d->pids);
			ioctl (d->fd, DVBLO_GET_SEC_PARAMETERS, &d->sec);
			if (event & EV_MASK_FE) {
#ifdef SHOW_EVENTS
				printf ("%p/%p Event Received: FE+Tuner/", d, d->r);
				if (event & EV_FRONTEND) {
					printf ("Frontend ");
				}
				if (event & EV_TUNER) {
					printf ("Tuner ");
				}
				if (event & EV_FREQUENCY) {
					printf ("Frequency:%d ", d->fe_parms.frequency);
				}
				if (event & EV_BANDWIDTH) {
					printf ("Bandwidth ");
				}
				printf ("\n");
#endif
				if (memcmp (&d->fe_parms, &old_fe_parms, sizeof (struct dvb_frontend_parameters))) {
					old_fe_parms = d->fe_parms;
					dbg ("fe_parms have changed!\n");
					need_tune = 1;
					if (d->ca_enable) {
						ci_cpl_clear (d->ci_slot);
					}
				}
			}
			if (event & EV_MASK_SEC) {
#ifdef SHOW_EVENTS
				printf ("%p/%p Event Received: SEC/", d, d->r);
				if (event & EV_TONE) {
					printf ("Tone:%d ", d->sec.tone_mode);
				}
				if (event & EV_VOLTAGE) {
					printf ("Voltage:%d ", d->sec.voltage);
				}
				if (event & EV_DISEC_MSG) {
					printf ("DISEC-Message:");
					int j;
					for (j = 0; j < d->sec.diseqc_cmd.msg_len; j++) {
						printf ("%02x ", d->sec.diseqc_cmd.msg[j]);
					}
				}
				if (event & EV_DISEC_BURST) {
					printf ("DISEC-Burst:%d ", d->sec.mini_cmd);
				}
				printf ("\n");
#endif
				if (d->sec.voltage == SEC_VOLTAGE_OFF) {
					recv_stop (d->r);
					memset (&old_fe_parms, 0, sizeof (struct dvb_frontend_parameters));
					need_tune = 0;
					dbg ("Stop %p\n", d->r);
				} else if (memcmp (&d->sec, &old_sec, sizeof (dvblo_sec_t))) {
					dbg ("SEC parms have changed!\n");
					old_sec = d->sec;
					need_tune = 1;
					if (d->ca_enable) {
						ci_cpl_clear (d->ci_slot);
					}
				}
			}
			if (event & EV_MASK_CA) {
#ifdef SHOW_EVENTS
				printf ("%p/%p Event Received: CA/", d, d->r);
#endif
				if (event & EV_CA_WRITE) {
#ifdef SHOW_EVENTS
					printf ("WRITE ");
#endif
					while (1) {
						if (!ioctl (d->fd, DVBLO_GET_TPDU, &tpdu) && tpdu.len) {
							if (d->c && d->ca_enable) {
								ci_pdu_t pdu;
								pdu.len = tpdu.len;
								pdu.data = tpdu.data;
								if (!cmd.reelcammode) {
									pdu.data[0] = d->ci_slot;
								}
								ci_write_pdu (d->c, &pdu);
								event |= EV_PIDFILTER;
							}
						} else {
							break;
						}
					}
				}
				if (event & EV_CA_RESET) {
#ifdef SHOW_EVENTS
					printf ("RESET ");
#endif
					last_ci_reset = time (NULL);
					if (d->ca_enable) {
						ci_cpl_clear (d->ci_slot);
					}
				}
#ifdef SHOW_EVENTS
				printf ("\n");
#endif
			}
			if (need_tune) {
				if (dvblo_tune (d, 1) == 2) {
					forced = time (NULL);
				}
				need_tune = 0;
			} else if (event & EV_MASK_PID) {
#ifdef SHOW_EVENTS
				printf ("%p/%p Event Received: Demux/", d, d->r);
				if (event & EV_PIDFILTER) {
					printf ("PID filter: %d pids", d->pids.num);
				}
				printf ("\n");
#endif
				forced = 0;
			}
		}
		if (forced) {
			if ((time (NULL) - forced) < TUNE_FORCED_TIMEOUT) {
				event &= ~EV_PIDFILTER;
			} else {
				event |= EV_PIDFILTER;
				forced = 0;
			}
		}
		if (event & EV_PIDFILTER) {
			dvblo_convert_pids (d);
			recv_pids (d->r, d->dstpids);
		}
		event = 0;
	}
	pthread_cleanup_pop (1);
	return NULL;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static dvblo_dev_t *dvblo_add (void)
{
	dvblo_dev_t *d = (dvblo_dev_t *) malloc (sizeof (dvblo_dev_t));

	if (!d)
		return NULL;
	memset (d, 0, sizeof (dvblo_dev_t));
	dvbmc_list_add_head (&devs.list, &d->list);
	return d;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void dvblo_del (dvblo_dev_t * d)
{
	d->recv_run = 0;
	if (pthread_exist(d->dvblo_recv_thread) && !pthread_cancel (d->dvblo_recv_thread)) {
		pthread_join (d->dvblo_recv_thread, NULL);
	}
	dvbmc_list_remove (&d->list);
	free (d);
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void dvblo_exit (void)
{
	dvblo_dev_t *d;
	dvblo_dev_t *dtmp;

	DVBMC_LIST_FOR_EACH_ENTRY_SAFE (d, dtmp, &devs.list, dvblo_dev_t, list) {
		dvblo_del (d);
	}
}

static dvblo_dev_t *find_dev_by_uuid (dvblo_dev_t * devs, char *uuid)
{
	dvblo_dev_t *d;
	DVBMC_LIST_FOR_EACH_ENTRY (d, &devs->list, dvblo_dev_t, list) {
		if (!strcmp (d->uuid, uuid)) {
			return d;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static int count_dev_by_type (dvblo_dev_t * devs, fe_type_t type)
{
	int ret = 0;
	dvblo_dev_t *d;
	DVBMC_LIST_FOR_EACH_ENTRY (d, &devs->list, dvblo_dev_t, list) {
		if (type == d->type) {
			ret++;
		}
	}
	return ret;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

satellite_list_t *dvblo_get_sat_list (char *SatelliteListName, netceiver_info_t * nci)
{
	int i;
	dbg ("looking for %s\n", SatelliteListName);
	for (i = 0; i < nci->sat_list_num; i++) {
		if (!strcmp (SatelliteListName, nci->sat_list[i].Name)) {
			dbg ("found uuid in sat list %d\n", i);
			return nci->sat_list + i;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static int diseqc_write_conf (char *disec_conf_path, char *rotor_conf_path, dvblo_dev_t * devs, int mode)
{
	int j, k;
	dvblo_dev_t *d;
	char buf[80];
	char tstr[16];
	FILE *f = NULL;
	FILE *fr = NULL;

	f = fopen (disec_conf_path, "wt");
	if (f == NULL) {
		return 0;
	}
	fprintf (f, "# diseqc.conf in VDR format auto generated\n\n");

	if (strlen(rotor_conf_path)) {
		special_status_mode=1;
		fr = fopen (rotor_conf_path, "wt");
	}
	if (fr) 
		fprintf (fr, "# rotor.conf auto generated\n\n");

	DVBMC_LIST_FOR_EACH_ENTRY (d, &devs->list, dvblo_dev_t, list) {
		satellite_list_t *sat_list = dvblo_get_sat_list (d->nci->tuner[d->tuner].SatelliteListName, d->nci);
		if (!sat_list) {
			continue;
		}
		for (j = 0; j < sat_list->sat_num; j++) {
			satellite_info_t *sat = sat_list->sat + j;
			for (k = 0; k < sat->comp_num; k++) {
				satellite_component_t *comp = sat->comp + k;
				int newpos = sat->SatPos ^ 1800;
				sprintf (buf, "e0 10 6f %02x %02x %02x", newpos & 0xff, (newpos >> 8) & 0xff, comp->Polarisation << 1 | !(comp->sec.tone_mode & 1));
				if (mode) {
					sprintf (tstr, "A%d ", d->device + 1);
				} else {
					tstr[0] = 0;
				}
				fprintf (f, "%s%s %d %c 0 [ %s ]\n", tstr, sat->Name, comp->RangeMax, comp->Polarisation == POL_H ? 'H' : 'V', buf);
			}
			fprintf (f, "\n");
			if (j==0 && fr && sat->type==SAT_SRC_ROTOR) {
				fprintf(fr, "%s %i %i %i %i %i\n", tstr, sat->SatPosMin, sat->SatPosMax, 
					sat->AutoFocus, sat->Latitude, sat->Longitude);
			}
		}
	}
	info ("created %s\n", disec_conf_path);
	fclose (f);
	if (fr) {
		info ("created %s\n", rotor_conf_path);
		fclose(fr);
	}
	return 1;
}

void dvblo_handler (void)
{
	int i;
	int n;
	int nci_num = 0;
	int write_conf = strlen (cmd.disec_conf_path);
	netceiver_info_list_t *nc_list = nc_get_list ();
	memset (&cacaps, 0, sizeof (dvblo_cacaps_t));

	info ("Device Type Limits: DVB-S: %d DVB-C: %d DVB-T: %d ATSC: %d DVB-S2: %d\n\n", cmd.tuner_type_limit[FE_QPSK], cmd.tuner_type_limit[FE_QAM], cmd.tuner_type_limit[FE_OFDM], cmd.tuner_type_limit[FE_ATSC], cmd.tuner_type_limit[FE_DVBS2]);

	while (dvblo_run) {
		nc_lock_list ();
		for (n = 0; n < nc_list->nci_num; n++) {
			netceiver_info_t *nci = nc_list->nci + n;
			for (i = 0; i < nci->tuner_num; i++) {
				char *uuid = nci->tuner[i].uuid;
				if (nci->tuner[i].preference < 0 || !strlen (uuid) || find_dev_by_uuid (&devs, uuid)) {
					//already seen
					continue;
				}
				fe_type_t type = nci->tuner[i].fe_info.type;

				if (type > FE_DVBS2) {
					continue;
				}
				if(dev_num >= MAX_DEVICES) {
					dbg("Limit dev_num reached limit of %d\n", MAX_DEVICES);
					continue;
				}
				if (count_dev_by_type (&devs, type) == cmd.tuner_type_limit[type]) {
					dbg ("Limit: %d %d>%d\n", type, count_dev_by_type (&devs, type), cmd.tuner_type_limit[type]);
					continue;
				}

				dvblo_dev_t *d = dvblo_add ();
				if (!d) {
					err ("Cannot get memory for dvb loopback context\n");
				}

				dbg ("allocate dev %d for uuid %s\n", dev_num, uuid);

				d->info = nci->tuner[i].fe_info;

				if (type == FE_DVBS2) {
					d->info.type = FE_QPSK;
				}

				strcpy (d->uuid, nci->tuner[i].uuid);
				if (!cmd.reelcammode) {
					if (cidev < CA_MAX_SLOTS && (cmd.ca_enable & (1 << dev_num))) {
						d->cacaps=(dvblo_cacaps_t*)((void *) &nci->ci);
						d->ca_enable = 1;
						dbg ("Enabling CA support for device %d\n", dev_num);
						char addrstr[INET6_ADDRSTRLEN];
						dvblo_get_nc_addr (addrstr, d->uuid, sizeof(addrstr));
						dbg ("dvblo_get_nc_addr: %s %s\n", addrstr, d->uuid);
						d->c = ci_find_dev_by_uuid (addrstr);
						if (d->c) {
							dbg ("Attaching ci dev %p to dvblo dev %p\n", d->c, d);
							ci_register_handler (d->c, cidev, dvblo_write_ci, d);
							d->ci_slot = cidev++;
						} else {
							dvblo_del (d);	//retry next time
							break;
						}
					} else {
						d->cacaps = &cacaps;
						d->ca_enable = 0;
						dbg ("Disabling CA support for device %d\n", dev_num);
					}
				} else {
					if (nci->ci.cap.slot_num && cmd.ca_enable) {
						d->ca_enable = 1;
						dbg ("Enabling CA support for device %d\n", dev_num);
						if (!cidev) {
							d->cacaps = (dvblo_cacaps_t*)((void *) &nci->ci);
							char addrstr[INET6_ADDRSTRLEN];
							dvblo_get_nc_addr (addrstr, d->uuid, sizeof(addrstr));
							dbg ("dvblo_get_nc_addr: %s %s\n", addrstr, d->uuid);
							d->c = ci_find_dev_by_uuid (addrstr);
							if (d->c) {
								dbg ("Attaching ci dev %p to dvblo dev %p\n", d->c, d);
								ci_register_handler (d->c, cidev++, dvblo_write_ci, d);
								ci_register_handler (d->c, cidev++, dvblo_write_ci, d);
							} else {
								dvblo_del (d);	//retry next time
								break;
							}
						} else {
							d->cacaps = &cacaps;
						}
					} else {
						d->cacaps = &cacaps;
						d->ca_enable = 0;
						dbg ("Disabling CA support for device %d\n", dev_num);
					}
				}

				d->r = recv_add ();
				if (!d->r) {
					err ("Cannot get memory for receiver\n");
				}

				d->device = dev_num++;
				d->type = type;
				d->tuner = i;
				d->nci = nci;

				register_ten_handler (d->r, dvblo_handle_ten, d);
				register_ts_handler (d->r, dvblo_handle_ts, d);

				info ("Starting thread for tuner UUID %s [%s] at device %d with type %d\n", d->uuid, nci->tuner[i].fe_info.name, nci->tuner[i].slot, nci->tuner[i].fe_info.type);
				int ret = pthread_create (&d->dvblo_recv_thread, NULL, dvblo_recv, d);
				while (!ret && !d->recv_run) {
					usleep (10000);
				}
				if (ret) {
					err ("pthread_create failed with %d\n", ret);
				}
			}
		}
		if (write_conf) {
			if (reload || (nci_num != nc_list->nci_num)) {
				nci_num = nc_list->nci_num;
				diseqc_write_conf (cmd.disec_conf_path, cmd.rotor_conf_path, &devs, cmd.vdrdiseqcmode);
				reload = 0;
			}
		}
		nc_unlock_list ();
		sleep (1);
	}
}
