/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

typedef struct
{
	netceiver_info_t *nci;
	int nci_num;
} netceiver_info_list_t;

DLL_SYMBOL netceiver_info_list_t *nc_get_list(void);
DLL_SYMBOL int nc_lock_list(void);
DLL_SYMBOL int nc_unlock_list(void);
void handle_tca (netceiver_info_t * nc_info);
