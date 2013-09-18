/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#include "headers.h"

int satellite_find_by_diseqc (satellite_reference_t * ref, recv_sec_t *sec, struct dvb_frontend_parameters *fep, int mode)
{
	int i, j, k, l;
	netceiver_info_list_t *nc_list=nc_get_list();
	char buf[6];
	memcpy(buf,"\xe0\x10\x6f\x0\x0\x0",6);
	int freq = fep->frequency/1000;
	int ret=0;
	int explicit_position=NO_SAT_POS;

	if (sec->diseqc_cmd.msg_len > 6 || !ref || !freq) {
		return 0;
	}

	for (l = 0; l < nc_list->nci_num; l++) {
		netceiver_info_t *nci = nc_list->nci + l;

		for (i = 0; i < nci->sat_list_num; i++) {
			satellite_list_t *sat_list = nci->sat_list + i;

			for (j = 0; j < sat_list->sat_num; j++) {
				satellite_info_t *sat = sat_list->sat + j;

				for (k = 0; k < sat->comp_num; k++) {
					satellite_component_t *comp = sat->comp + k;
					int oldpos=sat->SatPos^1800;
					int newpos=(sec->diseqc_cmd.msg[3]+(sec->diseqc_cmd.msg[4]<<8))^1800;
					// prepare synthetic messsage from satpos and pol./volt.
					buf[3]=oldpos;
					buf[4]=oldpos>>8;
					buf[5]=(comp->Polarisation&1)<<1 | !(comp->sec.tone_mode&1);
					dbg("compare: old/new %i/%i, %02x %02x %02x %02x %02x %02x <-> %02x %02x %02x %02x %02x %02x\n", oldpos, newpos,
					     buf[0]&0xff,buf[1]&0xff,buf[2]&0xff,buf[3]&0xff, buf[4]&0xff, buf[5]&0xff,
					     sec->diseqc_cmd.msg[0], sec->diseqc_cmd.msg[1], sec->diseqc_cmd.msg[2],
					     sec->diseqc_cmd.msg[3], sec->diseqc_cmd.msg[4], sec->diseqc_cmd.msg[5]);

					dbg("%i mode %i, len %i, %i > %i , %i < %i, %i < %i, %i > %i\n",sat->type,
					     mode, sec->diseqc_cmd.msg_len, freq,  comp->RangeMin, freq, comp->RangeMax,
					     sat->SatPosMin, newpos ,  sat->SatPosMax, newpos);

					// Check if coded sat pos matches
					if ((sat->type==SAT_SRC_LNB || sat->type==SAT_SRC_UNI)  && mode == 0 &&  sec->diseqc_cmd.msg_len>0 && 
					    (freq >= comp->RangeMin) && (freq <= comp->RangeMax) && 
					    !memcmp (buf, &sec->diseqc_cmd.msg, sec->diseqc_cmd.msg_len)) {
						dbg("Satpos MATCH\n");
						ret=1;
					}
					// check for rotor
					else if (sat->type==SAT_SRC_ROTOR && mode == 0 &&  sec->diseqc_cmd.msg_len>0 && 
						 (freq >= comp->RangeMin) && (freq <= comp->RangeMax) &&
						 (buf[5]==sec->diseqc_cmd.msg[5]) &&
						 (sat->SatPosMin<=newpos && sat->SatPosMax>=newpos)) {
						dbg("ROTOR MATCH %i\n",newpos);
						explicit_position=newpos;
						ret=1;						
					}
					// check if given diseqc matches raw tuner diseqc
					else if (mode == 1 && sec->diseqc_cmd.msg_len>0 && !memcmp (&comp->sec.diseqc_cmd.msg, &sec->diseqc_cmd.msg, sec->diseqc_cmd.msg_len)) {
						dbg("Diseqc 1.0 Match  %02x %02x %02x %02x %02x %02x\n",
						       comp->sec.diseqc_cmd.msg[0], comp->sec.diseqc_cmd.msg[1], comp->sec.diseqc_cmd.msg[2],
						       comp->sec.diseqc_cmd.msg[3], comp->sec.diseqc_cmd.msg[4], comp->sec.diseqc_cmd.msg[5]);
						ret=1;
					}else if (mode == 2 && (fe_sec_voltage_t)comp->Polarisation == sec->voltage && comp->sec.tone_mode== sec->tone_mode && comp->sec.mini_cmd == sec->mini_cmd) {
						dbg("Legacy Match, pol %i, tone %i, cmd %i\n",comp->Polarisation,comp->sec.tone_mode,comp->sec.mini_cmd);
						ret=1;
					}
					if (ret) {
						ref->netceiver = l;
						ref->sat_list = i;
						ref->sat = j;
						ref->comp = k;
						ref->position=explicit_position;
						info("Sat found: %d %d %d  %d, rotor %d\n",l,i,j,k, explicit_position);
						return ret;
					}
				}
			}
		}
	}
	return ret;
}

int satellite_get_pos_by_ref (satellite_reference_t * ref)
{
	netceiver_info_list_t *nc_list=nc_get_list();
	netceiver_info_t *nci = nc_list->nci + ref->netceiver;
	satellite_list_t *sat_list = nci->sat_list + ref->sat_list;
	satellite_info_t *sat = sat_list->sat + ref->sat;
	if (sat->type==SAT_SRC_ROTOR && ref->position!=NO_SAT_POS) {
		return ref->position;
	}
	return sat->SatPos;
}

int satellite_get_lof_by_ref (satellite_reference_t * ref)
{
	netceiver_info_list_t *nc_list=nc_get_list();
	netceiver_info_t *nci = nc_list->nci + ref->netceiver;
	satellite_list_t *sat_list = nci->sat_list + ref->sat_list;
	satellite_info_t *sat = sat_list->sat + ref->sat;
	satellite_component_t *comp = sat->comp + ref->comp;
	return comp->LOF;
}

recv_sec_t *satellite_find_sec_by_ref (satellite_reference_t * ref)
{
	netceiver_info_list_t *nc_list=nc_get_list();
	netceiver_info_t *nci = nc_list->nci + ref->netceiver;
	satellite_list_t *sat_list = nci->sat_list + ref->sat_list;
	satellite_info_t *sat = sat_list->sat + ref->sat;
	satellite_component_t *comp = sat->comp + ref->comp;
	return &comp->sec;
}

polarisation_t satellite_find_pol_by_ref (satellite_reference_t * ref)
{
	netceiver_info_list_t *nc_list=nc_get_list();
	netceiver_info_t *nci = nc_list->nci + ref->netceiver;
	satellite_list_t *sat_list = nci->sat_list + ref->sat_list;
	satellite_info_t *sat = sat_list->sat + ref->sat;
	satellite_component_t *comp = sat->comp + ref->comp;
	return comp->Polarisation;
}
