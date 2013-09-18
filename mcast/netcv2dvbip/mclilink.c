#include "dvbipstream.h"

#define BUFFER_SIZE (256*TS_PER_UDP*188)

#ifdef WIN32
#define CVT (char *)
#else
#define CVT
#endif

int gen_pat(unsigned char *buf, unsigned int program_number, 
	unsigned int pmt_pid, unsigned int ts_cnt)
{
	int pointer_field=0;
	int section_length=13;
	int transport_stream_id=0;
	int version_number=0;
	int current_next_indicator=1;
	int section_number=0;
	int last_section_number=0;
	int i=0;
	u_long crc; 
	
	buf[i++] = 0x47;
	buf[i++] = 0x40;
	buf[i++] = 0x00;
	buf[i++] = 0x10 | (ts_cnt&0xf);
	
	buf[i++] = pointer_field;
	buf[i++] = 0; // table_id
	buf[i] = 0xB0; // section_syntax_indicator=1, 0, reserved=11
	buf[i++]|= (section_length>>8)&0x0f;
	buf[i++] = section_length&0xff;

	buf[i++] = transport_stream_id>>8;
	buf[i++] = transport_stream_id&0xff;

	buf[i++] = 0xc0 | ((version_number&0x1f) << 1) | 
		(current_next_indicator&1);
	buf[i++] = section_number;
	buf[i++] = last_section_number;

	buf[i++] = program_number>>8;
	buf[i++] = program_number&0xff;
	buf[i++] = 0xe0 | ((pmt_pid>>8)&0x1F);
	buf[i++] = pmt_pid&0xff;
	
	crc=dvb_crc32 ((char *)buf+5, i-5);
	buf[i++] = (crc>>24)&0xff;
	buf[i++] = (crc>>16)&0xff;
	buf[i++] = (crc>>8)&0xff;
	buf[i++] = crc&0xff;
	
	for(;i<188;i++) {
		buf[i] = 0xff;
	}
	
//	printhex_buf ("PAT", buf, 188);
	return i;
}

/*-------------------------------------------------------------------------*/
int mcli_handle_ts (unsigned char *buffer, size_t len, void *p)
{
	stream_info_t *si = (stream_info_t *) p;
	
	if(si->stop)
		return len;
	
	int olen = len;

	int ret;
	unsigned int i;
	
again:	
	switch (si->si_state) {
	case 3:
	case 0:
		si->psi.start = 0;
		si->psi.len = 0;
		si->si_state++;
		goto again;

	case 1:
		ret = 0;
		for(i=0; i<len; i+=188) {
			ret = ts2psi_data (buffer+i, &si->psi, 188, 0);
			if(ret){
				break;
			}
		}
		if (ret < 0) {
			si->si_state = 0;
		}

		if (ret == 1) {
			if (!quiet)
				printf ("Channel: %s - Got PAT\n", 
					si->cdata->name);
			pmt_pid_list_t pat;
			ret = parse_pat_sect (si->psi.buf, si->psi.len, &pat);
			if (ret < 0) {
				si->si_state = 0;
			} else if (ret == 0) {
//				print_pat (&pat.p, pat.pl, pat.pmt_pids);
				unsigned int n;
				for (n = 0; n < pat.pmt_pids; n++) {
					if (pat.pl[n].program_number == 
					    (unsigned int)si->cdata->sid) {
						si->pmt_pid = 
						    pat.pl[n].network_pmt_pid;
						if (!quiet)
						    printf ("Channel: %s - SID "
							"%d has PMT Pid %d\n", 
							si->cdata->name, 
							si->cdata->sid, 
							si->pmt_pid);
						break;
					}
				}
				if (pat.pmt_pids) {
					free (pat.pl);
				}
				si->si_state++;
			}
		}
		break;
	case 4:
		ret = 0;
		for(i=0; i<len; i+=188) {
			ret = ts2psi_data(buffer+i, &si->psi, 188, si->pmt_pid);
			if(ret){
				break;
			}
		}
		if (ret < 0) {
			si->si_state = 2;
		}

		if (ret == 1) {
			if (!quiet)
				printf ("Channel: %s - Got PMT\n", 
					si->cdata->name);
			pmt_t hdr;
			si_ca_pmt_t pm, es;
			int es_pid_num;
//			printhex_buf ("Section", si->psi.buf, si->psi.len);
			si->fta=1;
			ret = parse_pmt_ca_desc (si->psi.buf, si->psi.len, 
				si->cdata->sid, &pm, &es, &hdr, &si->fta, 
				NULL, &es_pid_num);
			if (ret < 0) {
				si->si_state = 2;
			} else if (ret == 0) {
				si->es_pidnum = get_pmt_es_pids (es.cad, 
					es.size, si->es_pids, 1);
				if (si->es_pidnum <= 0) {
					si->si_state = 2;
				} else {
					si->si_state++;
				}
			}
			if (pm.size) {
				free (pm.cad);
			}
			if (es.size) {
				free (es.cad);
			}
			break;

		}
	case 6:
		/*
		for(i=0; i<len; i+=188) {
			ret = ts2psi_data (buffer+i, &si->psi, 188, 0x12);
			if(ret==1){
				printf("Channel: %s - Got EIT\n", 
					si->cdata->name);
			}
		}
		*/
		pthread_mutex_lock(&si->lock_ts);

		switch(len) {
		case 1*188:
		case 2*188:
		case 3*188:
		case 4*188:
		case 5*188:
		case 6*188:
		case 7*188:
			break;
		default:
			err(CVT "Channel: %s - bad data length %d, skipping\n",
				si->cdata->name,(int)len);
			goto out;
		}

		if(!si->wr) {
			pthread_mutex_lock(&si->lock_bf);
			if(si->free) {
				si->wr=si->free;
				si->free=si->free->next;
				pthread_mutex_unlock(&si->lock_bf);
			} else {
				pthread_mutex_unlock(&si->lock_bf);
				si->wr=(stream_buffer_t *)
					malloc(sizeof(stream_buffer_t));
				if(!si->wr) {
					err(CVT "Channel: %s - out of memory\n",
						si->cdata->name);
					goto out;
				}
			}
			si->wr->next=NULL;
			si->fill=0;
		}

		i=TS_PER_UDP*188-si->fill;

		if(len>i) {
			memcpy(si->wr->data+si->fill,buffer,i);

			pthread_mutex_lock(&si->lock_bf);
			if(!si->head)
				si->head=si->tail=si->wr;
			else {
				si->tail->next=si->wr;
				si->tail=si->wr;
			}
			if(si->free) {
				si->wr=si->free;
				si->free=si->free->next;
				pthread_mutex_unlock(&si->lock_bf);
			} else {
				pthread_mutex_unlock(&si->lock_bf);
				si->wr=(stream_buffer_t *)
					malloc(sizeof(stream_buffer_t));
				if(!si->wr) {
					err(CVT "Channel: %s - out of memory\n",
						si->cdata->name);
					goto out;
				}
			}
			si->wr->next=NULL;
			si->fill=0;
			buffer+=i;
			len-=i;
		}

		memcpy(si->wr->data+si->fill,buffer,len);
		si->fill+=len;

		if(si->fill==TS_PER_UDP*188) {
			pthread_mutex_lock(&si->lock_bf);
			if(!si->head)
				si->head=si->tail=si->wr;
			else {
				si->tail->next=si->wr;
				si->tail=si->wr;
			}
			pthread_mutex_unlock(&si->lock_bf);
			si->wr=NULL;
		}

out:		pthread_mutex_unlock(&si->lock_ts);
		break;
	}

	return olen;
}

/*-------------------------------------------------------------------------*/
int mcli_handle_ten (tra_t * ten, void *p)
{
	if(ten) {

		stream_info_t *si = (stream_info_t *) p;
		printf("Channel: %s - Status: %02X, Strength: %04X, SNR: %04X,"
			" BER: %04X\n", si->cdata->name, ten->s.st,
			ten->s.strength, ten->s.snr, ten->s.ber);
	}
	return 0;
}

#ifdef WIN32THREADS
DWORD WINAPI stream_watch(__in  LPVOID p)
#else
void *stream_watch (void *p)
#endif
{
	unsigned char ts[188];
	stream_info_t *si = (stream_info_t *) p;
	if (!quiet)
		printf("Channel: %s - stream watch thread started.\n", 
			si->cdata->name);
	while (1 ) {
		if(si->stop)
			break;
		if (si->pmt_pid && si->si_state == 2) {
			dvb_pid_t pids[3];
			memset (&pids, 0, sizeof (pids));
			pids[0].pid = si->pmt_pid;
			pids[1].pid = -1;
			if (!quiet)
				printf ("Channel: %s - Add PMT-PID: %d\n",
					si->cdata->name, si->pmt_pid);
			recv_pids (si->r, pids);
			si->si_state++;
		}
		if (si->es_pidnum && si->si_state == 5) {
			int i,k=0;
			size_t sz = sizeof(dvb_pid_t) * (si->es_pidnum+2 + 
				si->cdata->NumEitpids + si->cdata->NumSdtpids);
			dvb_pid_t *pids=(dvb_pid_t*)malloc(sz);
			if(pids==NULL) {
				err(CVT "Channel: %s - Can't get memory for "
					"pids\n", si->cdata->name);
				goto out;
			}
			memset (pids, 0, sz);
			pids[k++].pid = si->pmt_pid;
			//EIT PIDs
			for (i = 0; i < si->cdata->NumEitpids; i++)
			{
				pids[k++].pid = si->cdata->eitpids[i]; 
				if (!quiet)
					printf("Channel: %s - Add EIT-PID: "
						"%d\n", si->cdata->name, 
						si->cdata->eitpids[i]);
			}
			//SDT PIDs
			for (i = 0; i < si->cdata->NumSdtpids; i++)
			{
				pids[k++].pid = si->cdata->sdtpids[i]; 
				if (!quiet)
					printf("Channel: %s - Add SDT-PID: "
						"%d\n", si->cdata->name, 
						si->cdata->sdtpids[i]);
			}
			for (i = 0; i < si->es_pidnum; i++) {
				if (!quiet)
					printf ("Channel: %s - Add ES-PID: "
						"%d\n", si->cdata->name, 
						si->es_pids[i]);
				pids[i + k].pid = si->es_pids[i];
//				if(si->cdata->NumCaids) {
				if(!si->fta) {
					if (!quiet)
						printf("Channel: %s - %s\n", 
							si->cdata->name, 
							si->fta ? "Free-To-Air":
							"Crypted");
        				pids[i + k].id =  si->cdata->sid;
				}
				pids[i + k +1].pid = -1;
			}
			recv_pids (si->r, pids);
			free(pids);
			si->si_state++;
		}
		if(si->si_state == 6) {
			gen_pat(ts, si->cdata->sid, si->pmt_pid, si->ts_cnt++);
			mcli_handle_ts (ts, 188, si);
		}
out:		usleep (50000);
	}
	if (!quiet)
		printf("Channel: %s - stream watch thread stopped.\n", 
			si->cdata->name);
	return NULL;
}

/*-------------------------------------------------------------------------*/
void *mcli_stream_setup (const int channum)
{
	int cnum;
	stream_info_t *si;
	recv_info_t *r;
	struct dvb_frontend_parameters fep;
	recv_sec_t sec;
	dvb_pid_t pids[4];
	int source = -1; 
	fe_type_t tuner_type = FE_ATSC;

	cnum = channum-1;
//	printf ("mcli_stream_setup %i\n", cnum);
	if (cnum < 0 || cnum > get_channel_num ())
		return 0;

	si = (stream_info_t *) malloc (sizeof (stream_info_t));
	if (!si) {
		fprintf (stderr, "Cannot get memory for receiver\n");
		return NULL;
	}
	memset(si, 0, sizeof(stream_info_t));

	si->psi.buf = (unsigned char *) malloc (PSI_BUF_SIZE);
	if (!si->psi.buf) {
		fprintf (stderr, "Cannot get memory for receiver\n");
		free (si);
		return NULL;
	}

	r = recv_add ();
	if (!r) {
		fprintf (stderr, "Cannot get memory for receiver\n");
		free (si->psi.buf);
		free (si);
		return NULL;
	}
	si->r = r;

	pthread_mutex_init (&si->lock_bf, NULL);
	pthread_mutex_init (&si->lock_ts, NULL);

	if (!quiet)
		register_ten_handler (r, mcli_handle_ten, si);
	register_ts_handler (r, mcli_handle_ts, si);

	si->cdata = get_channel_data (cnum);

	memset (&fep, 0, sizeof (struct dvb_frontend_parameters));
	memset (&sec, 0, sizeof (recv_sec_t));

	fep.frequency = si->cdata->frequency;
	fep.inversion = INVERSION_AUTO;
        // DVB-S
	if (si->cdata->source >= 0) {
		fep.u.qpsk.symbol_rate = si->cdata->srate * 1000;
		fep.u.qpsk.fec_inner = (fe_code_rate_t)(si->cdata->coderateH | 
			(si->cdata->modulation<<16));
		fep.frequency *= 1000;
		fep.inversion = (fe_spectral_inversion_t)si->cdata->inversion;

		sec.voltage = (fe_sec_voltage_t)si->cdata->polarization;
		sec.mini_cmd = (fe_sec_mini_cmd_t)0;
		sec.tone_mode = (fe_sec_tone_mode_t)0;
		tuner_type = FE_QPSK;
		source = si->cdata->source;
	}
	// DVB-T
	else if (si->cdata->source == -2) {
		fep.u.ofdm.constellation = 
			(fe_modulation_t)si->cdata->modulation;
		fep.u.ofdm.code_rate_HP = (fe_code_rate_t)si->cdata->coderateH;
		fep.u.ofdm.code_rate_LP = (fe_code_rate_t)si->cdata->coderateL;
		fep.inversion = (fe_spectral_inversion_t)si->cdata->inversion;
		fep.u.ofdm.bandwidth = (fe_bandwidth_t)si->cdata->bandwidth;
		fep.u.ofdm.guard_interval = 
			(fe_guard_interval_t)si->cdata->guard;
		fep.u.ofdm.transmission_mode = 
			(fe_transmit_mode_t)si->cdata->transmission;
		fep.u.ofdm.hierarchy_information = 
			(fe_hierarchy_t)si->cdata->hierarchy;
		
		tuner_type = FE_OFDM;
		source = si->cdata->source;
	}
	// DVB-C
	else if (si->cdata->source == -3) {
		fep.u.qam.symbol_rate = si->cdata->srate * 1000;
		fep.u.qam.fec_inner = (fe_code_rate_t)si->cdata->coderateH;
		fep.u.qam.modulation = (fe_modulation_t)si->cdata->modulation;
		fep.frequency *= 1000;
		fep.inversion = (fe_spectral_inversion_t)si->cdata->inversion;

		tuner_type = FE_QAM;
		source = si->cdata->source;
	}



	memset (&pids, 0, sizeof (pids));

	pids[0].pid = 0;  // PAT
	pids[1].pid = -1;

	if (!quiet)
		printf ("Tuning: source: %s, frequency: %i, PAT pid %i, "
			"symbol rate %i\n", source>=0 ? "DVB-S(2)" : 
			source==-2 ? "DVB-T" : source==-3 ? "DVB-C" : "unknown",
			si->cdata->frequency, pids[0].pid, 
			fep.u.qpsk.symbol_rate);

	recv_tune (r, tuner_type, source, &sec, &fep, pids);

#ifdef WIN32THREADS
	CreateThread(NULL, 0, stream_watch, si, 0, NULL);
#else
	pthread_create (&si->t, NULL, stream_watch, si);
#endif
//	printf("mcli_setup %p\n",si);
	return si;
}

/*-------------------------------------------------------------------------*/
size_t mcli_stream_access (void *handle, char **buf)
{
	stream_info_t *si = (stream_info_t *) handle;

	if (!handle)
		return 0;
	if (si->stop)
		return 0;

	pthread_mutex_lock(&si->lock_bf);
	if(!si->head) {
		pthread_mutex_unlock(&si->lock_bf);
		return 0;
	}
	si->rd=si->head;
	si->head=si->head->next;
	pthread_mutex_unlock(&si->lock_bf);

	*buf=si->rd->data;
	return TS_PER_UDP*188;
}

/*-------------------------------------------------------------------------*/
size_t mcli_stream_part_access (void *handle, char **buf)
{
	stream_info_t *si = (stream_info_t *) handle;

	if (!handle)
		return 0;
	if (si->stop)
		return 0;

	pthread_mutex_lock(&si->lock_ts);
	if(!si->head) {
		int len;

		if(!si->wr)
			len=0;
		else {
			si->rd=si->wr;
			si->wr=NULL;
			len=si->fill;
			*buf=si->rd->data;
		}
		pthread_mutex_unlock(&si->lock_ts);
		return len;
	}
	si->rd=si->head;
	si->head=si->head->next;
	pthread_mutex_unlock(&si->lock_ts);

	*buf=si->rd->data;
	return TS_PER_UDP*188;
}

/*-------------------------------------------------------------------------*/
void mcli_stream_skip (void *handle)
{
	stream_info_t *si = (stream_info_t *) handle;

	if (!handle)
		return;

	pthread_mutex_lock(&si->lock_bf);
	si->rd->next=si->free;
	si->free=si->rd;
	pthread_mutex_unlock(&si->lock_bf);

	si->rd=NULL;
}

/*-------------------------------------------------------------------------*/
int mcli_stream_stop (void *handle)
{
	if (handle) {
		stream_info_t *si = (stream_info_t *) handle;
		recv_info_t *r = si->r;
		if (pthread_exist(si->t)) {
			si->stop = 1;
			pthread_join (si->t, NULL);
		}
		
		if (r) {
			pthread_mutex_lock(&si->lock_ts);
			register_ten_handler (r, NULL, NULL);
			register_ts_handler (r, NULL, NULL);
			pthread_mutex_unlock(&si->lock_ts);
			recv_stop(r);
			sleep(2);
			recv_del (r);
		}
		if (si->psi.buf) {
			free (si->psi.buf);
		}

		if (si->rd)
			free (si->rd);
		if (si->wr)
			free (si->wr);
		while (si->head) {
			stream_buffer_t *e;
			e = si->head;
			si->head = si->head->next;
			free (e);
		}
		while (si->free) {
			stream_buffer_t *e;
			e = si->free;
			si->free = si->free->next;
			free (e);
		}

		pthread_mutex_destroy(&si->lock_ts);
		pthread_mutex_destroy(&si->lock_bf);
		free (si);
	}
	return 0;
}

cmdline_t cmd = { 0 };

/*-------------------------------------------------------------------------*/
void mcli_startup (void)
{
#ifdef PTW32_STATIC_LIB
	pthread_win32_process_attach_np();
#endif
	netceiver_info_list_t *nc_list = nc_get_list ();

#if defined WIN32 || defined APPLE
	cmd.mld_start = 1;
#endif

//	printf ("Using Interface %s\n", cmd.iface);
	recv_init (cmd.iface, cmd.port);

	if (cmd.mld_start) {
		mld_client_init (cmd.iface);
	}
#if 1
	int n, i;
	printf ("Looking for netceivers out there....\n");
	while (1) {
		nc_lock_list ();
		for (n = 0; n < nc_list->nci_num; n++) {
			netceiver_info_t *nci = nc_list->nci + n;
			printf ("\nFound NetCeiver: %s\n", nci->uuid);
			if (!quiet)
			    for (i = 0; i < nci->tuner_num; i++) {
				printf ("  Tuner: %s, Type %d\n",
					nci->tuner[i].fe_info.name, 
					nci->tuner[i].fe_info.type);
			}
		}
		nc_unlock_list ();
		if (nc_list->nci_num) {
			break;
		}
		sleep (1);
	}
#endif
}
