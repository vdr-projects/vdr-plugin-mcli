/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

//#define DEBUG 1
#include "headers.h"

#ifdef DEBUG
const Param inversion_list[] = {
	{"INVERSION_OFF", INVERSION_OFF},
	{"INVERSION_ON", INVERSION_ON},
	{"INVERSION_AUTO", INVERSION_AUTO}
};

const Param bw_list[] = {
	{"BANDWIDTH_6_MHZ", BANDWIDTH_6_MHZ},
	{"BANDWIDTH_7_MHZ", BANDWIDTH_7_MHZ},
	{"BANDWIDTH_8_MHZ", BANDWIDTH_8_MHZ}
};

const Param fec_list[] = {
	{"FEC_1_2", FEC_1_2},
	{"FEC_2_3", FEC_2_3},
	{"FEC_3_4", FEC_3_4},
	{"FEC_4_5", FEC_4_5},
	{"FEC_5_6", FEC_5_6},
	{"FEC_6_7", FEC_6_7},
	{"FEC_7_8", FEC_7_8},
	{"FEC_8_9", FEC_8_9},
	{"FEC_AUTO", FEC_AUTO},
	{"FEC_NONE", FEC_NONE},
	{"FEC_1_4", FEC_1_4},	// RMM S2 Extension
	{"FEC_1_3", FEC_1_3},
	{"FEC_2_5", FEC_2_5},
	{"FEC_9_10", FEC_9_10}
};

const Param guard_list[] = {
	{"GUARD_INTERVAL_1_16", GUARD_INTERVAL_1_16},
	{"GUARD_INTERVAL_1_32", GUARD_INTERVAL_1_32},
	{"GUARD_INTERVAL_1_4", GUARD_INTERVAL_1_4},
	{"GUARD_INTERVAL_1_8", GUARD_INTERVAL_1_8}
};

const Param hierarchy_list[] = {
	{"HIERARCHY_1", HIERARCHY_1},
	{"HIERARCHY_2", HIERARCHY_2},
	{"HIERARCHY_4", HIERARCHY_4},
	{"HIERARCHY_NONE", HIERARCHY_NONE}
};

const Param constellation_list[] = {
	{"QPSK", QPSK},
	{"QAM_128", QAM_128},
	{"QAM_16", QAM_16},
	{"QAM_256", QAM_256},
	{"QAM_32", QAM_32},
	{"QAM_64", QAM_64},
	{"QPSK_S2", QPSK_S2},	// RMM S2 Extension
	{"PSK8", PSK8}
};

const Param transmissionmode_list[] = {
	{"TRANSMISSION_MODE_2K", TRANSMISSION_MODE_2K},
	{"TRANSMISSION_MODE_8K", TRANSMISSION_MODE_8K},
};

const Param capabilities_list[] = {
	{"Stupid: ", FE_IS_STUPID},
	{"FE_CAN_INVERSION_AUTO: ", FE_CAN_INVERSION_AUTO},
	{"CAN_FEC_1_2: ", FE_CAN_FEC_1_2},
	{"CAN_FEC_2_3: ", FE_CAN_FEC_2_3},
	{"CAN_FEC_3_4: ", FE_CAN_FEC_3_4},
	{"CAN_FEC_4_5: ", FE_CAN_FEC_4_5},
	{"CAN_FEC_6_7: ", FE_CAN_FEC_6_7},
	{"CAN_FEC_7_8: ", FE_CAN_FEC_7_8},
	{"CAN_FEC_8_9: ", FE_CAN_FEC_8_9},
	{"CAN_FEC_AUTO: ", FE_CAN_FEC_AUTO},
	{"FE_CAN_QPSK: ", FE_CAN_QPSK},
	{"FE_CAN_QAM_16: ", FE_CAN_QAM_16},
	{"FE_CAN_QAM_32: ", FE_CAN_QAM_32},
	{"FE_CAN_QAM_64: ", FE_CAN_QAM_64},
	{"FE_CAN_QAM_128: ", FE_CAN_QAM_128},
	{"FE_CAN_QAM_256: ", FE_CAN_QAM_256},
	{"FE_CAN_QAM_AUTO: ", FE_CAN_QAM_AUTO},
	{"FE_CAN_TRANSMISSION_MODE_AUTO: ", FE_CAN_TRANSMISSION_MODE_AUTO},
	{"FE_CAN_BANDWIDTH_AUTO: ", FE_CAN_BANDWIDTH_AUTO},
	{"FE_CAN_GUARD_INTERVAL_AUTO: ", FE_CAN_GUARD_INTERVAL_AUTO},
	{"FE_CAN_HIERARCHY_AUTO: ", FE_CAN_HIERARCHY_AUTO},
	{"FE_CAN_MUTE_TS:  ", FE_CAN_MUTE_TS}
//      {"FE_CAN_CLEAN_SETUP: ",FE_CAN_CLEAN_SETUP}
};

#define LIST_SIZE(x) sizeof(x)/sizeof(Param)

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void print_fe_info (struct dvb_frontend_info *fe_info)
{
	fprintf (stdout, "-------------------------------------------\n");
	fprintf (stdout, "Tuner name: %s\n", fe_info->name);
	fprintf (stdout, "Tuner type: %u\n", (unsigned int) fe_info->type);
	fprintf (stdout, "Frequency min.: %u\n", fe_info->frequency_min);
	fprintf (stdout, "Frequency max.: %u\n", fe_info->frequency_max);
	fprintf (stdout, "Frequency stepsize: %u\n", fe_info->frequency_stepsize);
	fprintf (stdout, "Frequency tolerance: %u\n", fe_info->frequency_tolerance);
	fprintf (stdout, "Symbol rate min: %u\n", fe_info->symbol_rate_min);
	fprintf (stdout, "Symbol rate max: %u\n", fe_info->symbol_rate_max);
	fprintf (stdout, "Symbol rate tolerance: %u\n", fe_info->symbol_rate_tolerance);
	fprintf (stdout, "Notifier delay: %u\n", fe_info->notifier_delay);
	fprintf (stdout, "Cpas: 0x%x\n", (unsigned int) fe_info->caps);

	fprintf (stdout, "-------------------------------------------\n");
	fprintf (stdout, "Frontend Capabilities:\n");
	int i;

	for (i = 0; i < LIST_SIZE (capabilities_list); i++) {
		if (fe_info->caps & capabilities_list[i].value)
			fprintf (stdout, "%syes\n", capabilities_list[i].name);
		else
			fprintf (stdout, "%sno\n", capabilities_list[i].name);
	}
	fprintf (stdout, "-------------------------------------------\n");
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void print_frontend_settings (struct dvb_frontend_parameters *frontend_param)
{
	int i;
	fprintf (stdout, "\n----- Front End Settings ----- ");
	fprintf (stdout, "\nFrequency               : %u \n", frontend_param->frequency);
	for (i = 0; i < LIST_SIZE (inversion_list); i++) {
		if (inversion_list[i].value == frontend_param->inversion)
			fprintf (stdout, "Inversion               : %s\n", inversion_list[i].name);

	}
	//
	for (i = 0; i < LIST_SIZE (bw_list); i++) {
		if (frontend_param->u.ofdm.bandwidth == bw_list[i].value)
			fprintf (stdout, "Bandwidth               : %s\n", bw_list[i].name);

	}
	for (i = 0; i < LIST_SIZE (fec_list); i++) {
		if (fec_list[i].value == frontend_param->u.ofdm.code_rate_HP)
			fprintf (stdout, "Code Rate HP            : %s\n", fec_list[i].name);

	}
	for (i = 0; i < LIST_SIZE (fec_list); i++) {
		if (fec_list[i].value == frontend_param->u.ofdm.code_rate_LP)
			fprintf (stdout, "Code Rate LP            : %s\n", fec_list[i].name);

	}

	for (i = 0; i < LIST_SIZE (constellation_list); i++) {
		if (constellation_list[i].value == frontend_param->u.ofdm.constellation)
			fprintf (stdout, "Modulation              : %s\n", constellation_list[i].name);

	}

	for (i = 0; i < LIST_SIZE (transmissionmode_list); i++) {
		if (transmissionmode_list[i].value == frontend_param->u.ofdm.transmission_mode)
			fprintf (stdout, "Transmission mode       : %s\n", transmissionmode_list[i].name);

	}

	for (i = 0; i < LIST_SIZE (guard_list); i++) {
		if (guard_list[i].value == frontend_param->u.ofdm.guard_interval)
			fprintf (stdout, "Guard interval          : %s\n", guard_list[i].name);

	}

	for (i = 0; i < LIST_SIZE (hierarchy_list); i++) {
		if (hierarchy_list[i].value == frontend_param->u.ofdm.hierarchy_information)
			fprintf (stdout, "Hierarchy Information   : %s\n", hierarchy_list[i].name);

	}

}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void print_mcg (struct in6_addr *mcg)
{
	char host[80];
	unsigned int freq;
	struct in6_addr mc;
	int i;

	for (i = 0; i < 8; i++) {
		mc.s6_addr16[i] = ntohs (mcg->s6_addr16[i]);
	}

	freq = mc.s6_addr16[6] | (mc.s6_addr16[7] & NOPID_MASK) << 3;

	inet_ntop (AF_INET6, mcg->s6_addr, (char *) host, INET6_ADDRSTRLEN);
	fprintf (stdout, "MCG: %s\n", host);

	fprintf (stdout, "\n");
	fprintf (stdout, "TS-Streaming group\n");
	fprintf (stdout, "-----------------------------\n");
	fprintf (stdout, "Streaming Group - 0x%x \n", (mc.s6_addr16[1] >> 12) & 0xf);
	fprintf (stdout, "Priority - 0x%x \n", (mc.s6_addr16[1] >> 8) & 0xf);
	fprintf (stdout, "Reception System - 0x%x \n", mc.s6_addr16[1] & 0xff);
	fprintf (stdout, "CAM Handling - 0x%x \n", mc.s6_addr16[2]);
	fprintf (stdout, "Polarisation - 0x%x \n", (mc.s6_addr16[3] >> 12) & 0xf);
	fprintf (stdout, "SATPosition - 0x%x \n", mc.s6_addr16[3] & 0xfff);
	fprintf (stdout, "Symbol Rate - 0x%x \n", mc.s6_addr16[4]);
	fprintf (stdout, "Modulation - 0x%x \n", mc.s6_addr16[5]);
	fprintf (stdout, "Frequency (0x%x) - %d / %d\n\n", freq, freq * (16667 / 8), freq * (250 / 8));

	fprintf (stdout, "PID - 0x%x \n", mc.s6_addr16[7] & PID_MASK);
}
#endif
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/* Frequency 19Bit
   DVB-T/DVB-C 524288 Steps * (25/12)kHz = 0...1092MHz in 2.083333kHz steps  
   DVB-S       524288 Steps * (1/20) MHz = 0...26.2GHz in 50kHz steps
*/
void fe_parms_to_mcg (struct in6_addr *mcg, streaming_group_t StreamingGroup, fe_type_t type, recv_sec_t * sec, struct dvb_frontend_parameters *fep, int vpid)
{
	int i;
	unsigned int Priority = 0;
	unsigned int ReceptionSystem = 0;
	unsigned int CAMHandling = 0;
	unsigned int Polarisation = 0;
	unsigned int SATPosition = NO_SAT_POS;
	unsigned int Symbolrate = 0;
	unsigned int Modulation = 0;
	unsigned int TransmissionMode = 0;
	unsigned int Frequency;
	double fmul;

	// Default for DVB-T and DVB-C
	fmul = 12.0 * (((double) fep->frequency) + 1041);
	Frequency = (unsigned int) (fmul / 25000.0);

	switch ((int)type) {
	case FE_QPSK:
	case FE_DVBS2:
		Frequency = (fep->frequency + 24) / 50;
		//sec->diseqc_cmd currently not used
		// Fixme: Translation Diseqc->position/LOF-frequency
		Polarisation = (sec->mini_cmd << 3) | (sec->tone_mode << 2) | sec->voltage;
		Symbolrate = fep->u.qpsk.symbol_rate / 1000;
		Modulation |= (fep->u.qpsk.fec_inner) & 0xf;

		// RMM S2 extension: Put Modulation in 23:16 and rolloff in 31:24
		if (((fep->u.qpsk.fec_inner >> 16) & 0xff) == PSK8)
			Modulation |= 2 << 4;
		if (((fep->u.qpsk.fec_inner >> 16) & 0xff) == QPSK_S2)
			Modulation |= 1 << 4;
		Modulation |= fep->inversion << 14;
		break;
	case FE_QAM:
		Symbolrate = fep->u.qam.symbol_rate / 200;
		Modulation |= fep->u.qam.modulation;
		Modulation |= fep->inversion << 14;
		break;
	case FE_OFDM:
		TransmissionMode = fep->u.ofdm.transmission_mode;
		Symbolrate = (TransmissionMode & 0x7) << 8 | (fep->u.ofdm.code_rate_HP << 4) | fep->u.ofdm.code_rate_LP;
		Modulation |= (fep->u.ofdm.constellation & 0xf) | (fep->u.ofdm.hierarchy_information & 3) << 4 | (fep->u.ofdm.bandwidth & 3) << 7 | (fep->u.ofdm.guard_interval & 7) << 9 | (fep->inversion & 3) << 14;
		break;
	case FE_ATSC:
		Modulation |= fep->u.vsb.modulation;
		Modulation |= fep->inversion << 14;
		break;
	}
	
	if (type == FE_DVBS2 && !(Modulation & 0x30) ){
		type=FE_QPSK;
	}

	ReceptionSystem = type;

	mcg->s6_addr16[0] = MC_PREFIX;
	mcg->s6_addr16[1] = ((StreamingGroup & 0xf) << 12) | ((Priority & 0xf) << 8) | (ReceptionSystem & 0xff);
	mcg->s6_addr16[2] = CAMHandling;
	mcg->s6_addr16[3] = ((Polarisation & 0xf) << 12) | (SATPosition & 0xfff);
	mcg->s6_addr16[4] = Symbolrate;
	mcg->s6_addr16[5] = Modulation;
	mcg->s6_addr16[6] = Frequency;
	mcg->s6_addr16[7] = (vpid & PID_MASK) | ((Frequency >> 16) << 13);

	for (i = 0; i < 8; i++) {
		mcg->s6_addr16[i] = htons (mcg->s6_addr16[i]);
	}
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int mcg_to_all_parms(struct in6_addr *mcg, struct mcg_data * mcd)
{
	int ret;
	mcd->mcg=*mcg;
	int n;

	ret=mcg_to_fe_parms(mcg, &mcd->type, &mcd->sec, &mcd->fep, &mcd->vpid);

	if (ret)
		return ret;
	mcg_get_satpos(mcg, &mcd->satpos);

	for(n=0;n<MAX_TUNER_CACHE;n++) {
		mcd->sat_cache[n].resolved=NOT_RESOLVED;
		mcd->sat_cache[n].num=0;
		mcd->sat_cache[n].component=0;
	}

	return 0;
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int mcg_to_fe_parms (struct in6_addr *mcg, fe_type_t * type, recv_sec_t * sec, struct dvb_frontend_parameters *fep, int *vpid)
{
	struct in6_addr mc = *mcg;
	streaming_group_t StreamingGroup;
	unsigned int freq;
	double fmul;
	fe_type_t fetype;

	int i;
	for (i = 0; i < 8; i++) {
		mc.s6_addr16[i] = ntohs (mc.s6_addr16[i]);
	}

	StreamingGroup = (streaming_group_t)((mc.s6_addr16[1] >> 12) & 0xf);

	if (StreamingGroup != STREAMING_PID) {
		return -1;
	}

	if (fep) {
		memset (fep, 0, sizeof (struct dvb_frontend_parameters));
	}
	if (sec) {
		memset (sec, 0, sizeof (recv_sec_t));
	}


	freq = mc.s6_addr16[6] | ((mc.s6_addr16[7] & NOPID_MASK) << 3);

	fmul = 25000.0 * (double) freq;

	fep->frequency = (unsigned int) (fmul / 12.0);
	fep->inversion = (fe_spectral_inversion_t)((mc.s6_addr16[5] >> 14) & 3);
	fetype = (fe_type_t)(mc.s6_addr16[1] & 0xff);

	if (type) {
		*type = fetype;
	}
	switch ((int)fetype) {
	case FE_QPSK:
	case FE_DVBS2:
		{
			int Polarisation = mc.s6_addr16[3] >> 12;
			fep->frequency = freq * 50;
			sec->mini_cmd = (fe_sec_mini_cmd_t)((Polarisation >> 3) & 1);
			sec->tone_mode = (fe_sec_tone_mode_t)((Polarisation >> 2) & 1);
			sec->voltage = (fe_sec_voltage_t)(Polarisation & 3);

			fep->u.qpsk.symbol_rate = mc.s6_addr16[4] * 1000;
			fep->u.qpsk.fec_inner = (fe_code_rate_t)(mc.s6_addr16[5] & 0xf);

			unsigned int fec_inner=(unsigned int)fep->u.qpsk.fec_inner;

			// RMM S2 Extension
			switch (mc.s6_addr16[5] & 0x30) {
			case 0x10:
				fec_inner |= QPSK_S2 << 16;
				fep->u.qpsk.fec_inner=(fe_code_rate_t)fec_inner;
				*type = (fe_type_t) FE_DVBS2;	// force FE type
				break;
			case 0x20:
				fec_inner |= PSK8 << 16;
				fep->u.qpsk.fec_inner=(fe_code_rate_t)fec_inner;
				*type = (fe_type_t) FE_DVBS2;
				break;
			default:
				 *type = FE_QPSK;
			}
		}
		break;
	case FE_QAM:
		fep->u.qam.symbol_rate = mc.s6_addr16[4] * 200;
		fep->u.qam.modulation = (fe_modulation_t)(mc.s6_addr16[5] & 0xf);
		// Ignore inversion
		break;
	case FE_OFDM:
		fep->u.ofdm.transmission_mode = (fe_transmit_mode_t)((mc.s6_addr16[4] >> 8) & 3);
		fep->u.ofdm.code_rate_HP = (fe_code_rate_t)((mc.s6_addr16[4] >> 4) & 0xf);
		fep->u.ofdm.code_rate_LP = (fe_code_rate_t)(mc.s6_addr16[4] & 0xf);

		fep->u.ofdm.constellation = (fe_modulation_t) (mc.s6_addr16[5] & 0xf);
		fep->u.ofdm.hierarchy_information = (fe_hierarchy_t)((mc.s6_addr16[5] >> 4) & 3);
		fep->u.ofdm.bandwidth = (fe_bandwidth_t)((mc.s6_addr16[5] >> 7) & 3);
		fep->u.ofdm.guard_interval = (fe_guard_interval_t)((mc.s6_addr16[5] >> 9) & 7);
		break;
	case FE_ATSC:
		fep->u.vsb.modulation = (fe_modulation_t)(mc.s6_addr16[5] & 0xf);
		break;
	}

	if (vpid) {
		*vpid = mc.s6_addr16[7] & PID_MASK;
	}
	//print_frontend_settings(fep);
	return 0;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void mcg_set_streaming_group (struct in6_addr *mcg, streaming_group_t StreamingGroup)
{
	int i;

	for (i = 0; i < 8; i++) {
		mcg->s6_addr16[i] = ntohs (mcg->s6_addr16[i]);
	}

	// Change StreamingGroup      
	mcg->s6_addr16[1] = ((StreamingGroup & 0xf) << 12) | (mcg->s6_addr16[1] & 0x0fff);

	// Remove PID
	mcg->s6_addr16[7] &= NOPID_MASK;
	
	// Remove CAID
	mcg->s6_addr16[2] = 0;

	for (i = 0; i < 8; i++) {
		mcg->s6_addr16[i] = htons (mcg->s6_addr16[i]);
	}
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void mcg_get_streaming_group (struct in6_addr *mcg, streaming_group_t *StreamingGroup)
{
	if(StreamingGroup) {
		*StreamingGroup=(streaming_group_t)((ntohs (mcg->s6_addr16[1]) >> 12) & 0xf);
	}
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void mcg_set_pid (struct in6_addr *mcg, int pid)
{

	mcg->s6_addr16[7] = ntohs (mcg->s6_addr16[7]);

	// Remove PID
	mcg->s6_addr16[7] &= NOPID_MASK;

	// Set new PID
	mcg->s6_addr16[7] |= pid;

	mcg->s6_addr16[7] = htons (mcg->s6_addr16[7]);
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void mcg_get_pid (struct in6_addr *mcg, int *pid)
{
	if (pid) {
		*pid=ntohs (mcg->s6_addr16[7]) & PID_MASK;
	}
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void mcg_init_streaming_group (struct in6_addr *mcg, streaming_group_t StreamingGroup)
{
	unsigned int Priority = 1;
	mcg->s6_addr16[0] = MC_PREFIX;
	mcg->s6_addr16[1] = ((StreamingGroup & 0xf) << 12) | ((Priority & 0xf) << 8) | (0 & 0xff);
	mcg->s6_addr16[2] = 0;
	mcg->s6_addr16[3] = 0;
	mcg->s6_addr16[4] = 0;
	mcg->s6_addr16[5] = 0;
	mcg->s6_addr16[6] = 0;
	mcg->s6_addr16[7] = 0;
	int i;
	for (i = 0; i < 8; i++) {
		mcg->s6_addr16[i] = htons (mcg->s6_addr16[i]);
	}

}

void mcg_get_priority (struct in6_addr *mcg, int *priority)
{
	if (priority) {
		*priority = (ntohs (mcg->s6_addr16[1])>>8) & 0xf;
	}
}

void mcg_set_priority (struct in6_addr *mcg, int priority)
{
	mcg->s6_addr16[1] = ntohs (mcg->s6_addr16[1]);
	mcg->s6_addr16[1] &= 0xf0ff;
	mcg->s6_addr16[1] |= (priority & 0xf) << 8;
	mcg->s6_addr16[1] = htons (mcg->s6_addr16[1]);
}

void mcg_get_satpos (struct in6_addr *mcg, int *satpos)
{
	if (satpos) {
		*satpos = ntohs (mcg->s6_addr16[3]) & 0xfff;
	}
}

void mcg_set_satpos (struct in6_addr *mcg, int satpos)
{
	mcg->s6_addr16[3] = ntohs (mcg->s6_addr16[3]);

	// Remove SatPos
	mcg->s6_addr16[3] &= ~NO_SAT_POS;

	// Set new SatPos
	mcg->s6_addr16[3] |= (satpos & NO_SAT_POS);

	mcg->s6_addr16[3] = htons (mcg->s6_addr16[3]);
}

void mcg_get_id (struct in6_addr *mcg, int *id)
{
	if (id) {
		*id = ntohs (mcg->s6_addr16[2]);
	}
}

void mcg_set_id (struct in6_addr *mcg, int id)
{
	mcg->s6_addr16[2] = htons(id);
}

#if defined LIBRARY || defined SERVER
#ifndef OS_CODE
	#define OS_CODE 3
#endif
#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif

static unsigned char gzip_hdr[] = { 0x1f, 0x8b, Z_DEFLATED, 0 /*flags */ , 0, 0, 0, 0 /*time */ , 0 /*xflags */ , OS_CODE };

int check_header (const Bytef * buf, unsigned int buflen)
{
	if (buflen <= 10)
		return 0;

	if (buf[0] != gzip_hdr[0] || buf[1] != gzip_hdr[1]) {
		return -1;
	}

	if (memcmp (buf, gzip_hdr, sizeof (gzip_hdr))) {
		return -2;
	}
	return 10;
}

unsigned int get32_lsb_first (unsigned char *ptr)
{
	int i;
	unsigned int val = 0;
	for (i = 3; i >= 0; i--) {
		val <<= 8;
		val |= (ptr[i] & 0xff);
	}
	return val;
}

void put32_lsb_first (unsigned char *ptr, unsigned int val)
{
	int i;
	for (i = 0; i < 4; i++) {
		ptr[i] = val & 0xff;
		val >>= 8;
	}
}

int gzip_ (Bytef * dest, unsigned int *destLen, const Bytef * source, unsigned int sourceLen, int level)
{
	unsigned int crc = crc32 (0L, Z_NULL, 0);
	z_stream stream;
	int err;

	if (*destLen <= 10) {
		return Z_BUF_ERROR;
	}
	memcpy (dest, gzip_hdr, sizeof (gzip_hdr));

	stream.next_in = (Bytef *) source;
	stream.avail_in = sourceLen;

	stream.next_out = dest + 10;
	stream.avail_out = *destLen - 10;

	stream.zalloc = (alloc_func) 0;
	stream.zfree = (free_func) 0;
	stream.opaque = (voidpf) 0;

	err = deflateInit2 (&stream, level, Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);

	if (err != Z_OK)
		return err;

	err = deflate (&stream, Z_FINISH);
	if (err != Z_STREAM_END) {
		deflateEnd (&stream);
		return err == Z_OK ? Z_BUF_ERROR : err;
	}
	*destLen = stream.total_out + 10;

	err = deflateEnd (&stream);
	crc = crc32 (crc, source, sourceLen);

	put32_lsb_first ((unsigned char *) (dest + *destLen), crc);
	put32_lsb_first ((unsigned char *) (dest + *destLen + 4), sourceLen);

	*destLen += 8;
	return err;
}

int gzip (Bytef * dest, unsigned int *destLen, const Bytef * source, unsigned int sourceLen, int level)
{
	if (!level) {
		memcpy (dest, source, sourceLen);
		*destLen = sourceLen;
		return 0;
	}
	return gzip_ (dest, destLen, source, sourceLen,level);
}

int gunzip_ (Bytef * dest, unsigned int *destLen, const Bytef * source, unsigned int sourceLen)
{
	unsigned int crc = crc32 (0L, Z_NULL, 0);
	z_stream stream;
	int err;
	int ret = check_header (source, sourceLen);
	if (ret < 0) {
		return ret;
	}

	stream.next_in = (Bytef *) source + ret;
	stream.avail_in = sourceLen - ret;

	stream.next_out = dest;
	stream.avail_out = *destLen;

	stream.zalloc = (alloc_func) 0;
	stream.zfree = (free_func) 0;

	err = inflateInit2 (&stream, -MAX_WBITS);
	if (err != Z_OK)
		return err;

	err = inflate (&stream, Z_FINISH);
	if (err != Z_STREAM_END) {
		inflateEnd (&stream);
		if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && stream.avail_in == 0))
			return Z_DATA_ERROR;
		return err;
	}
	*destLen = stream.total_out;

	err = inflateEnd (&stream);
	crc = crc32 (crc, dest, stream.total_out);

	int crc_found = get32_lsb_first ((unsigned char *) (stream.next_in));
	int len_found = get32_lsb_first ((unsigned char *) (stream.next_in + 4));

	if (crc_found == crc && len_found == stream.total_out) {
		return err;
	}

	return Z_DATA_ERROR;
}

int gunzip (Bytef * dest, unsigned int *destLen, const Bytef * source, unsigned int sourceLen)
{
	int ret = gunzip_ (dest, destLen, source, sourceLen);
	if (ret == -1) {
		memcpy (dest, source, sourceLen);
		*destLen = sourceLen;
		return 0;
	} else if (ret < 0) {
		return -1;
	}
	return 0;
}
#endif
#ifndef BACKTRACE
	void print_trace (void)
	{
	}
#else
#include <execinfo.h>
/* Obtain a backtrace and print it to stdout. */
void print_trace (void)
{
	void *array[10];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace (array, 10);
	strings = backtrace_symbols (array, size);

	printf ("Obtained %zd stack frames.\n", size);

	for (i = 0; i < size; i++) {
		printf ("%s\n", strings[i]);
	}
	free (strings);
}


void SignalHandlerCrash(int signum)
{
	void *array[15];
	size_t size;
	char **strings;
	size_t i;
	FILE *f;
	char dtstr[16];
	time_t t=time(NULL);
	struct tm *tm=localtime(&t);

	signal(signum,SIG_DFL); // Allow core dump

	f=fopen("/var/log/mcli.crashlog","a");
	if (f) {
		strftime(dtstr, sizeof(dtstr), "%b %e %T", tm);
		size = backtrace (array, 15);
		strings = backtrace_symbols (array, size);
		fprintf(f,"%s ### Crash signal %i ###\n",dtstr, signum);
		for (i = 0; i < size; i++)
			fprintf (f, "%s Backtrace %i: %s\n", dtstr, i, strings[i]);
		free (strings);
		fclose(f);
	}
}
#endif

#ifdef SYSLOG
pthread_mutex_t _loglock = PTHREAD_MUTEX_INITIALIZER;

UDPContext * syslog_fd = NULL;
char *_logstr = NULL;

int syslog_init(void)
{
	struct in6_addr mcglog;
	mcg_init_streaming_group (&mcglog, STREAMING_LOG);
	syslog_fd = server_udp_open (&mcglog, 23000, NULL);
	if(syslog_fd) {
		_logstr=(char *)malloc(10240);
	}
	
	return syslog_fd?0:-1;
}

int syslog_write(char *s)
{
	return udp_write (syslog_fd, (uint8_t *)s, strlen(s));
}

void syslog_exit(void)
{
	if(syslog_fd) {
		udp_close(syslog_fd);
		free(_logstr);
	}
}
#endif
