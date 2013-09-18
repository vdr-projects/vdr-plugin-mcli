/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

typedef struct {
	struct list list;

	pthread_t ci_recv_thread;
	char uuid[UUID_SIZE];
	SOCKET fd_ci;
	int recv_run;
	int device;
	int connected;
	recv_cacaps_t *cacaps;
	u_int8_t *txdata;
	u_int8_t *rxdata;
	int (*handle_ci_slot[CA_MAX_SLOTS]) (ci_pdu_t *tpdu, void *context);
	void *handle_ci_slot_context[CA_MAX_SLOTS];
} ci_dev_t;

DLL_SYMBOL int ci_register_handler(ci_dev_t *c, int slot, int (*p) (ci_pdu_t *, void *), void *context);
DLL_SYMBOL int ci_unregister_handler(ci_dev_t *c, int slot);
DLL_SYMBOL int ci_write_pdu(ci_dev_t *c, ci_pdu_t *tpdu);
DLL_SYMBOL ci_dev_t *ci_find_dev_by_uuid (char *uuid);
DLL_SYMBOL int ci_init (int ca_enable, char *intf, int p);
DLL_SYMBOL void ci_exit (void);
