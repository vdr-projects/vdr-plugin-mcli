/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#include "headers.h"

static netceiver_info_list_t nc_list;
static pthread_mutex_t nci_lock=PTHREAD_MUTEX_INITIALIZER;

static netceiver_info_t *nci_find_unique (netceiver_info_list_t * ncl, char *uuid)
{
	int i;
	for (i = 0; i < ncl->nci_num; i++) {
		if (!strcmp (ncl->nci[i].uuid, uuid)) {
			return ncl->nci + i;
		}
	}
	return NULL;
}

static void nci_free(netceiver_info_t * nc_info)
{
	int i, j;
	for (i = 0; i < nc_info->sat_list_num; i++) {
		satellite_list_t *sat_list = nc_info->sat_list + i;

		for (j = 0; j < sat_list->sat_num; j++) {
			satellite_info_t *sat = sat_list->sat + j;

			free (sat->comp);
		}
		free (sat_list->sat);
	}
	free (nc_info->sat_list);
	free (nc_info->tuner);
}

static int nci_add_unique (netceiver_info_list_t * ncl, netceiver_info_t * nci)
{
	netceiver_info_t *ncf=nci_find_unique (ncl, nci->uuid);
	if (!ncf) {
		ncl->nci = (netceiver_info_t *) realloc (ncl->nci, sizeof (netceiver_info_t) * (ncl->nci_num + 1));
		if (!ncl->nci) {
			err ("Cannot get memory for netceiver_info\n");
		}
		memcpy (ncl->nci + ncl->nci_num, nci, sizeof (netceiver_info_t));
		(ncl->nci+ncl->nci_num)->lastseen = time(NULL);
		ncl->nci_num++;
		return 1;
	} else {
		nci_free(ncf);
		memcpy(ncf, nci, sizeof (netceiver_info_t));
		ncf->lastseen = time(NULL);
	}
	return 0;
}

netceiver_info_list_t *nc_get_list(void)
{
	return &nc_list;
}

int nc_lock_list(void) 
{
	return pthread_mutex_lock (&nci_lock);	
}

int nc_unlock_list(void) 
{
	return pthread_mutex_unlock (&nci_lock);
}

void handle_tca (netceiver_info_t * nc_info)
{
	nc_lock_list();
	if (nci_add_unique (&nc_list, nc_info)) {
		dbg ("New TCA from %s added\n", nc_info->uuid);
	}
	nc_unlock_list();
}
