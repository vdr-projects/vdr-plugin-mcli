/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

//#define TESTING
#ifdef TESTING
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>

#include "ciparser.h"
static unsigned char ll[] = { 0x00, 0x01, 0xA0, 0x28, 0x01, 0x90, 0x02, 0x00, 0x05, 0x9F, 0x80, 0x32, 0x1F, 0x03, 0x32, 0xC9, 0x01, 0x00, 0x0F, 0x01, 0x09, 0x06, 0x17, 0x62, 0xE0, 0x65, 0x00, 0x09, 0x09, 0x04, 0x17, 0x02, 0xE1, 0x2D, 0x02, 0x00, 0xA0, 0x00, 0x00, 0x04, 0x00, 0xA1, 0x00, 0x00 };
static unsigned char lr[] = { 0x00, 0x01, 0x80, 0x02, 0x01, 0x80 };
static unsigned char la[] = { 0x00, 0x01, 0xA0, 0x07, 0x01, 0x91, 0x04, 0x00, 0x01, 0x00, 0x41, 0x80, 0x02, 0x01, 0x00 };
static unsigned char lb[] = { 0x00, 0x01, 0xA0, 0x82, 0x00, 0x17, 0x01, 0x90, 0x02, 0x00, 0x03, 0x9F, 0x80, 0x31, 0x0E, 0x06, 0x02, 0x06, 0x02, 0x17, 0x02, 0x17, 0x62, 0x01, 0x00, 0x05, 0x00, 0x18, 0x00, 0x80, 0x02, 0x01, 0x00 };
static unsigned char lc[] = { 0x01, 0x02, 0xA0, 0x5F, 0x02, 0x90, 0x02, 0x00, 0x06, 0x9F, 0x80, 0x32, 0x56, 0x03, 0x03, 0x8B, 0x01, 0x00, 0x00, 0x02, 0x00, 0xA3, 0x00, 0x23, 0x01, 0x09, 0x0F, 0x05, 0x00, 0xE2, 0xC3, 0x10, 0x01, 0x00, 0x13, 0x01, 0x20, 0x14, 0x03, 0x00, 0x94, 0x0D, 0x09, 0x0F, 0x05, 0x00, 0xE2, 0xCD, 0x10, 0x01, 0x00, 0x13, 0x01, 0x20, 0x14, 0x03, 0x02, 0x38, 0x08, 0x04, 0x00, 0x5C, 0x00, 0x23, 0x01, 0x09, 0x0F, 0x05, 0x00, 0xE2, 0xC3, 0x10, 0x01, 0x00, 0x13, 0x01, 0x20, 0x14, 0x03, 0x00, 0x94, 0x0D, 0x09, 0x0F, 0x05, 0x00, 0xE2, 0xCD, 0x10, 0x01, 0x00, 0x13, 0x01, 0x20, 0x14, 0x03, 0x02, 0x38, 0x08 };
static unsigned char ld[] = { 0x00, 0x01, 0xA0, 0x82, 0x00, 0x10, 0x01, 0x90, 0x02, 0x00, 0x03, 0x9F, 0x80, 0x33, 0x07, 0x2D, 0xB9, 0x01, 0x81, 0x00, 0x08, 0x00, 0x80, 0x02, 0x01, 0x00 };
static unsigned char le[] = { 0x00, 0x01, 0xA0, 0x34, 0x01, 0x90, 0x02, 0x00, 0x03, 0x9F, 0x80, 0x32, 0x2B, 0x03, 0x00, 0x0B, 0x01, 0x00, 0x11, 0x01, 0x09, 0x06, 0x17, 0x22, 0xF0, 0x0B, 0x00, 0x0B, 0x09, 0x06, 0x17, 0x02, 0xF0, 0x0B, 0x00, 0x0B, 0x02, 0x06, 0xFF, 0x00, 0x00, 0x04, 0x07, 0x00, 0x00, 0x00, 0x04, 0x07, 0x01, 0x00, 0x00, 0x00, 0x07, 0x03, 0x00, 0x00};

#define dbg(format, arg...) printf("%s:%d " format , __FILE__ , __LINE__ , ## arg)
#define err(format, arg...) {printf("err:%s:%d: %s (%d): " format , __FILE__ , __LINE__ ,strerror(errno), errno, ## arg);print_trace();abort();}
#define info(format, arg...) printf("%s:%d: " format , __FILE__ , __LINE__ ,## arg)
#define warn(format, arg...) printf("%s:%d: " format , __FILE__ , __LINE__ ,## arg)
#define STATIC 
#else
//#define DEBUG
#include "headers.h"
#endif

#define CA_MAX_CAIDS 16
#define CA_MAX_PIDS 16
#ifndef CA_MAX_SLOTS
#define CA_MAX_SLOTS 3
#endif
typedef struct
{
	u_int16_t caid[CA_MAX_CAIDS];
	u_int16_t pid[CA_MAX_PIDS];
	u_int16_t capid[CA_MAX_PIDS];
} caid_pid_list_t;

static caid_pid_list_t cpl[CA_MAX_SLOTS];

STATIC void dump(u_int8_t *data, int len)
{
#ifdef DEBUG
	int j;
	printf("Dump: ");
	for(j=0;j<len;j++) {
		printf("%02x ",data[j]);
	}
	printf("\n");
#endif
}

STATIC int ci_cpl_find_pid (int slot, int pid)
{
	if (slot < 0 || slot >= CA_MAX_SLOTS) {
		return -1;
	}
	int i;
	for (i = 0; i < CA_MAX_PIDS; i++) {
		if (pid == cpl[slot].pid[i])
			return 1;
	}
	return 0;
}

STATIC int ci_cpl_find_caid (int slot, int caid)
{
	if (slot < 0 || slot >= CA_MAX_SLOTS) {
		return -1;
	}
	int i;
	for (i = 0; i < CA_MAX_CAIDS; i++) {
		if (caid == cpl[slot].caid[i])
			return 1;
	}
	return 0;
}

STATIC int ci_cpl_find_capid (int slot, int pid)
{
	if (slot < 0 || slot >= CA_MAX_SLOTS) {
		return -1;
	}
	int i;
	for (i = 0; i < CA_MAX_PIDS; i++) {
		if (pid == cpl[slot].capid[i])
			return 1;
	}
	return 0;
}

STATIC int ci_cpl_delete_pid (int slot, int pid)
{
	if (slot < 0 || slot >= CA_MAX_SLOTS) {
		return -1;
	}
	int i;
	for (i = 0; i < CA_MAX_PIDS; i++) {
		if (cpl[slot].pid[i]==pid) {
			cpl[slot].pid[i] = 0;
			dbg ("-------> Slot: %d Deleted pid: %04x\n", slot, pid);
			return 1;
		}
	}
	return 0;
}

STATIC int ci_cpl_update_pid (int slot, int pid)
{
	if (slot < 0 || slot >= CA_MAX_SLOTS) {
		return -1;
	}
	if (!ci_cpl_find_pid (slot, pid)) {
		int i;
		for (i = 0; i < CA_MAX_PIDS; i++) {
			if (!cpl[slot].pid[i]) {
				cpl[slot].pid[i] = pid;
				dbg ("-------> Slot: %d Added   pid: %04x\n", slot, pid);
				return 1;
			}
		}
	}
	return 0;
}

STATIC int ci_cpl_update_caid (int slot, int caid)
{
	if (slot < 0 || slot >= CA_MAX_SLOTS) {
		return -1;
	}
	if (!ci_cpl_find_caid (slot, caid)) {
		int i;
		for (i = 0; i < CA_MAX_CAIDS; i++) {
			if (!cpl[slot].caid[i]) {
				cpl[slot].caid[i] = caid;
				dbg ("-------> Slot: %d Added  caid: %04x\n", slot, caid);
				return 1;
			}
		}
	}
	return 0;
}

STATIC int ci_cpl_update_capid (int slot, int capid)
{
	if (slot < 0 || slot >= CA_MAX_SLOTS) {
		return -1;
	}
	if (!ci_cpl_find_capid (slot, capid)) {
		int i;
		for (i = 0; i < CA_MAX_PIDS; i++) {
			if (!cpl[slot].capid[i]) {
				cpl[slot].capid[i] = capid;
				dbg ("-------> Slot: %d Added capid: %04x\n", slot, capid);
				return 1;
			}
		}
	}
	return 0;
}

int ci_cpl_find_caid_by_pid (int pid)
{
	int i;
	int slot;
	
	if(!pid) {
		return 0;
	}
	for (slot = 0; slot < CA_MAX_SLOTS; slot++) {
		for (i = 0; i < CA_MAX_PIDS; i++) {
			if (pid == cpl[slot].pid[i]) {
				return cpl[slot].caid[0];
			}
		}
	}
	return 0;
}

int ci_cpl_find_slot_by_caid_and_pid (int caid, int pid)
{
	int slot;
	for (slot = 0; slot < CA_MAX_SLOTS; slot++) {
		if (ci_cpl_find_pid (slot, pid) && ci_cpl_find_caid (slot, caid)) {
			return slot;
		}
	}
	return -1;
}

int ci_cpl_clear (int slot)
{
	if (slot < 0 || slot >= CA_MAX_SLOTS) {
		return -1;
	}
	memset (&cpl[slot], 0, sizeof (caid_pid_list_t));
	return 0;
}

int ci_cpl_clear_pids (int slot)
{
	if (slot < 0 || slot >= CA_MAX_SLOTS) {
		return -1;
	}
	memset (cpl[slot].pid, 0, sizeof (u_int16_t) * CA_MAX_PIDS);
	return 0;
}

int ci_cpl_clear_caids (int slot)
{
	if (slot < 0 || slot >= CA_MAX_SLOTS) {
		return -1;
	}
	memset (cpl[slot].caid, 0, sizeof (u_int16_t) * CA_MAX_CAIDS);
	return 0;
}

int ci_cpl_clear_capids (int slot)
{
	if (slot < 0 || slot >= CA_MAX_SLOTS) {
		return -1;
	}
	memset (cpl[slot].capid, 0, sizeof (u_int16_t) * CA_MAX_PIDS);
	return 0;
}

STATIC int ci_decode_length (unsigned int *len, u_int8_t * v)
{
	int ret = 0;

	if (*v & LENGTH_SIZE_INDICATOR) {
		int l = *v & 0x7f;
		if (l > 4) {
			return -1;
		}
		ret = l + 1;
		*len = 0;
		while (l--) {
			v++;
			*len <<= 8;
			*len |= *v;
		}
	} else {
		*len = *v;
		ret = 1;
	}
	return ret;
}

#if 0
STATIC int ci_decode_al_ca_info (ci_al_t * al)
{
	int i = 0;
	u_int8_t *data = al->data;
	int len = al->length;

	if (len & 1) {
		dbg ("ci_decode_al_ca_info: invalid length %d\n", len);
	}

	len >>= 1;

	u_int16_t *caid = (u_int16_t *) malloc (sizeof (u_int16_t) * len);
	ci_cpl_clear_caids (al->sl->tl->ll->slot);
	while (i < len) {
		caid[i++] = ntohs16 (data);
		data += 2;
		ci_cpl_update_caid (al->sl->tl->ll->slot, caid[i - 1]);
		dbg ("CAID[%d]: %04x\n", i - 1, caid[i - 1]);
	}
	if (caid) {
		free (caid);
	}
	return data - al->data;
}
#endif

STATIC int ca_decode_ca_descr (ca_desc_t ** cadescr, int count, u_int8_t * data, int len, int *magic)
{
	*cadescr = (ca_desc_t *) realloc (*cadescr, sizeof (ca_desc_t *) * (count + 1));
	if (!*cadescr) {
		err ("ca_decode_ca_descr: out of memory\n");
	}
	ca_desc_t *c = *cadescr + count;

//      u_int8_t descriptor_tag = *data;
	data++;
	u_int8_t descriptor_length = *data;
	data++;
	c->ca_id = ntohs16 (data);
	data += 2;
	c->ca_pid = ntohs16 (data);
	data += 2;
	dbg ("cadescr: %p %d ca_id: %04x ca_pid: %04x\n", cadescr, count, c->ca_id, c->ca_pid);
	if(magic && c->ca_id > 0 && c->ca_id < 3 && c->ca_pid > 0 && c->ca_pid < 3 && c->ca_id == c->ca_pid){
		*magic = c->ca_id;
	}
	return descriptor_length + 2;
}


STATIC int ci_decode_al_ca_pmt (ci_al_t * al)
{
	ca_pmt_t p;
	int magic = 0;
	int slot = 0;
	int cleared = 0;

	memset (&p, 0, sizeof (ca_pmt_t));

	int ret;
	u_int8_t *data = al->data;
	int len;
	
	p.ca_pmt_list_management = *data;
	data++;

	p.program_number = ntohs16 (data);
	data += 2;

	p.version_number = *data;
	data++;

	p.program_info_length = (u_int16_t) ntohs16 (data);
	data += 2;

	dbg ("ci_decode_al_ca_pmt: ca_pmt_list_management:%02x program_number:%04x version_number:%02x program_info_length:%04x\n", p.ca_pmt_list_management, p.program_number, p.version_number, p.program_info_length);
	if (p.program_info_length) {
		int ca_descr_count = 0;
		len = p.program_info_length - 1;
		p.ca_pmt_cmd_id = *data;
		dbg ("p.ca_pmt_cmd_id:%02x\n", p.ca_pmt_cmd_id);
		data++;
		while (len>0) {
			ret = ca_decode_ca_descr (&p.cadescr, ca_descr_count, data, len, &magic);
			if (magic)
				slot = magic - 1;
			else
				slot = al->sl->tl->ll->slot;
			if (!cleared) {
				if(p.ca_pmt_list_management == CPLM_ONLY || p.ca_pmt_list_management == CPLM_FIRST || p.ca_pmt_list_management == CPLM_UPDATE) {
		                	ci_cpl_clear_pids(slot);
		                	ci_cpl_clear_capids(slot);
		        	        ci_cpl_clear_caids(slot);
					cleared = 1;
			        }
			}
			if (ret < 0) {
				warn ("error decoding ca_descriptor\n");
				break;
			}
			if((magic != p.cadescr[ca_descr_count].ca_id) || (magic != p.cadescr[ca_descr_count].ca_pid)){
				ci_cpl_update_caid (slot, p.cadescr[ca_descr_count].ca_id);
				ci_cpl_update_capid (slot, p.cadescr[ca_descr_count].ca_pid);
                        }
			ca_descr_count++;
			data += ret;
			len -= ret;
		}
		if (p.cadescr) {
			free (p.cadescr);
		}
	}

	len = al->length - (data - al->data);
	int pidn = 0;

	while (len>0) {
		p.pidinfo = (pidinfo_t *) realloc (p.pidinfo, sizeof (pidinfo_t) * (pidn + 1));
		if (!p.pidinfo) {
			err ("ci_decode_al_ca_pmt: out of memory");
		}
		memset (&p.pidinfo[pidn], 0, sizeof (pidinfo_t));
		p.pidinfo[pidn].stream_type = *data;
		data++;
		len--;
		p.pidinfo[pidn].pid = ntohs16 (data);
		data += 2;
		len -= 2;
		p.pidinfo[pidn].es_info_length = ntohs16 (data);
		data += 2;
		len -= 2;

		dbg ("len: %d count: %d, stream_type: %02x, pid: %04x es_info_length: %04x\n", len, pidn, p.pidinfo[pidn].stream_type, p.pidinfo[pidn].pid, p.pidinfo[pidn].es_info_length);
		if (p.pidinfo[pidn].es_info_length) {
			int pi_len = p.pidinfo[pidn].es_info_length - 1;
			p.pidinfo[pidn].ca_pmt_cmd_id = *data;
			data++;
			len--;
			int pid_ca_descr_count = 0;
			while (pi_len>0) {
				ret = ca_decode_ca_descr (&p.pidinfo[pidn].cadescr, pid_ca_descr_count, data, pi_len, NULL);
	                        if (!cleared) {
        	                        if(p.ca_pmt_list_management == CPLM_ONLY || p.ca_pmt_list_management == CPLM_FIRST || p.ca_pmt_list_management == CPLM_UPDATE) {
                	                        ci_cpl_clear_pids(slot);
                        	                ci_cpl_clear_capids(slot);
                                	        ci_cpl_clear_caids(slot);
                                        	cleared = 1;
                                	}
                        	}
				if (ret < 0) {
					warn ("error decoding ca_descriptor\n");
					break;
				}
	                        if((magic != p.pidinfo[pidn].cadescr[pid_ca_descr_count].ca_id) || (magic  != p.pidinfo[pidn].cadescr[pid_ca_descr_count].ca_pid)){
			                ci_cpl_update_pid (slot, p.pidinfo[pidn].pid);
					ci_cpl_update_caid (slot, p.pidinfo[pidn].cadescr[pid_ca_descr_count].ca_id);
					ci_cpl_update_capid (slot, p.pidinfo[pidn].cadescr[pid_ca_descr_count].ca_pid);
				}
				pid_ca_descr_count++;
				data += ret;
				pi_len -= ret;
				len -= ret;
			}
		}
		if (p.pidinfo[pidn].cadescr) {
			free (p.pidinfo[pidn].cadescr);
		}
		pidn++;
	}
	if (p.pidinfo) {
		free (p.pidinfo);
	}
	return 0;
}

STATIC int ci_decode_al_ca_pmt_reply (ci_al_t * al)
{
	ca_pmt_reply_t p;

	memset (&p, 0, sizeof (ca_pmt_reply_t));

	u_int8_t *data = al->data;
	int len;

	p.program_number = ntohs16 (data);
	data += 2;

	p.version_number = *data;
	data++;

	p.ca_enable = *data;
	data++;

	len = al->length - (data - al->data);
	int pidn = 0;

	dbg ("ci_decode_al_ca_pmt_reply: program_number: %04x ca_enable: %02x\n", p.program_number, p.ca_enable);

	while (len>0) {
		p.pidcaenable = (pid_ca_enable_t *) realloc (p.pidcaenable, sizeof (pid_ca_enable_t) * (pidn + 1));
		if (!p.pidcaenable) {
			err ("ci_decode_al_ca_pmt_reply: out of memory\n");
		}
		memset (&p.pidcaenable[pidn], 0, sizeof (pid_ca_enable_t));
		p.pidcaenable[pidn].pid = ntohs16 (data);
		data += 2;
		p.pidcaenable[pidn].ca_enable = *data;
		data++;
		len -= 3;
		if ((p.pidcaenable[pidn].ca_enable == CPCI_OK_DESCRAMBLING) || (p.pidcaenable[pidn].ca_enable == CPCI_OK_MMI) || (p.pidcaenable[pidn].ca_enable == CPCI_QUERY)) {
			ci_cpl_update_pid (al->sl->tl->ll->slot, p.pidcaenable[pidn].pid);
		} else {
			ci_cpl_delete_pid (al->sl->tl->ll->slot, p.pidcaenable[pidn].pid);
		}
		dbg ("count: %d pid: %04x pid_ca_enable: %02x\n", pidn, p.pidcaenable[pidn].pid, p.pidcaenable[pidn].ca_enable);
		pidn++;
	}
	if (p.pidcaenable) {
		free (p.pidcaenable);
	}
	return 0;
}

STATIC int ci_decode_al (ci_sl_t * sl)
{
	int ret = 0;
	int done = 0;
	int len = 3;
	ci_al_t al;
	u_int8_t *data = sl->data;
	al.sl = sl;

	al.tag = 0;
	while (len--) {
		al.tag <<= 8;
		al.tag |= *data;
		data++;
	}
	done += 3;
	ret = ci_decode_length (&al.length, data);
	if (ret < 0) {
		warn ("ci_decode_al ci_decode_length failed\n");
		return ret;
	}

	data += ret;
	done += ret;
	//Fake
	al.data = data;

	dbg ("ci_decode_al: tag:%03x length: %02x data[0]:%02x done: %02x\n", al.tag, al.length, al.data[0], done);

	switch (al.tag) {
	case AOT_CA_INFO:
//		ci_decode_al_ca_info (&al);
		break;
	case AOT_CA_PMT:
		ci_decode_al_ca_pmt (&al);
		break;
	case AOT_CA_PMT_REPLY:
		ci_decode_al_ca_pmt_reply (&al);
		break;
	}

	done += al.length;

	dbg ("ci_decode_al: done %02x\n", done);
	return done;
}

STATIC int ci_decode_sl (ci_tl_t * tl)
{
	int ret = 0;
	int done = 0;
	unsigned int len;
	ci_sl_t sl;
	u_int8_t *data = tl->data;
	sl.tl = tl;

	sl.tag = *data;
	data++;
	done++;
	
	if(sl.tag != ST_SESSION_NUMBER) {
		return tl->length;
	}

	ret = ci_decode_length (&len, data);
	if (ret < 0) {
		warn ("ci_decode_sl ci_decode_length failed\n");
		return ret;
	}
	data += ret;
	done += ret;

	if (len > 4) {
		warn ("invalid length (%d) for session_object_value\n", len);
		return -1;
	}

	sl.object_value = 0;
	while (len--) {
		sl.object_value <<= 8;
		sl.object_value |= *data;
		data++;
		done++;
	}

	sl.data = data;
	sl.length = tl->length - done;

	while (sl.length>0) {
		dbg ("ci_decode_sl: object_value:%02x length: %02x done: %02x\n", sl.object_value, sl.length, done);
		ret = ci_decode_al (&sl);
		if (ret < 0) {
			warn ("ci_decode_al failed\n");
			return ret;
		}
		sl.length -= ret;
		sl.data += ret;
		done += ret;
	}
	dbg ("ci_decode_sl: done %02x\n", done);
	return done;
}

STATIC int ci_decode_tl (ci_ll_t * ll)
{
	int ret = 0;
	int done = 0;
	ci_tl_t tl;
	u_int8_t *data = ll->data;
	tl.ll = ll;

	tl.c_tpdu_tag = *data;
	data++;
	done++;

	ret = ci_decode_length (&tl.length, data);
	if (ret < 0) {
		warn ("ci_decode_tl ci_decode_length failed\n");
		return ret;
	}

	data += ret;
	done += ret;

	tl.tcid = *data;
	data++;
	done++;

	if (tl.tcid != ll->tcid) {
		warn ("Error: redundant tcid mismatch %02x %02x\n",tl.tcid, ll->tcid);
		return -1;
	}

	tl.data = data;

	//According to A.4.1.1
	tl.length--;

	while (tl.length>0) {
		dbg ("ci_decode_tl: c_tpdu_tag:%02x tcid:%02x length: %02x done: %02x\n", tl.c_tpdu_tag, tl.tcid, tl.length, done);
		if (tl.c_tpdu_tag == T_DATA_LAST || tl.c_tpdu_tag == T_DATA_MORE) {
			ret = ci_decode_sl (&tl);
			if (ret < 0) {
				warn ("ci_decode_sl failed\n");
				return ret;
			}
		} else {
			ret = tl.length;
		}
		tl.length -= ret;
		tl.data += ret;
		done += ret;
	}
	dbg ("ci_decode_tl: done %02x\n", done);
	return done;
}

int ci_decode_ll (uint8_t * tpdu, int len)
{
	int ret = 0;
	int done = 0;
	u_int8_t *data=tpdu;
	ci_ll_t ll;
	dump(tpdu,len);
	
	ll.slot = *data;
	data++;

	ll.tcid = *data;
	data++;

	ll.data = data;
	ll.length = len - (data-tpdu);

	while (ll.length) {

		dbg ("ci_decode_ll: slot:%02x tcid:%02x length: %02x\n", ll.slot, ll.tcid, ll.length);
		ret = ci_decode_tl (&ll);
		if (ret < 0) {
			warn ("ci_decode_tl failed\n");
			return ret;
		}
		ll.length -= ret;
		ll.data += ret;
	}
	dbg ("ci_decode_ll: done %02x\n", len);
	return done;
}

#ifdef TESTING
int main (int argc, char **argv)
{
	int ret;

	printf ("ci_decode_ll len: %02x\n", sizeof (lb));
	ret = ci_decode_ll (lb, sizeof (lb));
	printf ("ci_decode_ll ret: %02x\n", ret);

	printf ("ci_decode_ll len: %02x\n", sizeof (ll));
	ret = ci_decode_ll (ll, sizeof (ll));
	printf ("ci_decode_ll ret: %02x\n", ret);


	printf ("ci_decode_ll len: %02x\n", sizeof (ld));
	ret = ci_decode_ll (ld, sizeof (ld));
	printf ("ci_decode_ll ret: %02x\n", ret);

	printf ("ci_decode_ll len: %02x\n", sizeof (lc));
	ret = ci_decode_ll (lc, sizeof (lc));
	printf ("ci_decode_ll ret: %02x\n", ret);

	printf ("ci_decode_ll len: %02x\n", sizeof (le));
	ret = ci_decode_ll (le, sizeof (le));
	printf ("ci_decode_ll ret: %02x\n", ret);

//	printf ("caid %04x for pid %04x\n", ci_cpl_find_caid_by_pid (0x5c), 0x5c);
	return 0;
}
#endif
