/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#include "headers.h"

static tra_info_t tra_list;

static tra_t *tra_find_unique (tra_info_t *trl, char *uuid)
{
	int i;
	for (i = 0; i < trl->tra_num; i++) {
		if (!strcmp (trl->tra[i].uuid, uuid)) {
			return trl->tra + i;
		}
	}
	return NULL;
}

static int tra_add_unique (tra_info_t *trl, tra_t *tri)
{
	tra_t *trf=tra_find_unique (trl, tri->uuid);
	if (!trf) {
		trl->tra = (tra_t *) realloc (trl->tra, sizeof (tra_t) * (trl->tra_num + 1));
		if (!trl->tra) {
			err ("Cannot get memory for netceiver_info\n");
		}
		trf = trl->tra + trl->tra_num;
		trl->tra_num++;
	} 
	memcpy (trf, tri, sizeof (tra_t));
	return 1;
}

tra_info_t *tra_get_list(void)
{
	return &tra_list;
}

int handle_tra(tra_info_t *tra_info)
{
	int i;
	if(tra_info->tra_num) {
		for (i = 0; i < tra_info->tra_num; i++) {
			tra_add_unique (&tra_list, tra_info->tra+i);
		}
		memcpy(tra_list.cam, tra_info->cam, MAX_CAMS*sizeof(cam_info_t));
		free (tra_info->tra);
		return 1;
	}
	return 0;
}
