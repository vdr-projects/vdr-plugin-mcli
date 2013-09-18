/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#include "headers.h"

//#define SHOW_TPDU 

static ci_dev_t devs;
static int dev_num = 0;
static int ci_run = 0;
static pthread_t ci_handler_thread;
static int port = 23000;
static char iface[IFNAMSIZ];

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static int ci_connect (ci_dev_t * c)
{
	int ret;
	int j;
	struct in6_addr nc;

	if (c->connected) {
		return 0;
	}

	if (c->fd_ci) {
		closesocket (c->fd_ci);
	}

	c->fd_ci = socket (PF_INET6, SOCK_STREAM, 0);
	j = 1;
	if (setsockopt (c->fd_ci, SOL_SOCKET, SO_REUSEADDR, (_SOTYPE) & j, sizeof (j)) < 0) {
		warn ("setsockopt REUSEADDR\n");
	}

	j = 1;
	if (setsockopt (c->fd_ci, SOL_SOCKET, TCP_NODELAY, (_SOTYPE) & j, sizeof (j)) < 0) {
		warn ("setsockopt TCP_NODELAY\n");
	}

	inet_pton (AF_INET6, c->uuid, &nc);
#ifdef	SHOW_TPDU
	char host[INET6_ADDRSTRLEN];
	inet_ntop (AF_INET6, &nc, (char *) host, INET6_ADDRSTRLEN);
	info ("Connect To: %s\n", host);
#endif
	struct sockaddr_in6 addr;
	memset (&addr, 0, sizeof (struct sockaddr_in6));

	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons (port);
	addr.sin6_addr = nc;
	addr.sin6_scope_id = if_nametoindex (iface);

	ret = connect (c->fd_ci, (struct sockaddr *) &addr, sizeof (struct sockaddr_in6));
	if (ret < 0) {
		warn ("Failed to access NetCeiver CA support\n");
	} else {
		c->connected = 1;
	}
	return ret;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int ci_write_pdu (ci_dev_t * c, ci_pdu_t * tpdu)
{
	int ret = -1;

	dbg ("ci_write_pdu: %p %d\n", tpdu->data, tpdu->len);

	ci_decode_ll (tpdu->data, tpdu->len);
	memcpy (c->txdata + 2, tpdu->data, tpdu->len);
	c->txdata[0] = tpdu->len >> 8;
	c->txdata[1] = tpdu->len & 0xff;
	if (!ci_connect (c)) {
#ifdef SHOW_TPDU
		int j;
		info ("Send TPDU: ");
		for (j = 0; j < tpdu->len; j++) {
			info ("%02x ", tpdu->data[j]);
		}
		info ("\n");
#endif
		ret = send (c->fd_ci, (char *) c->txdata, tpdu->len + 2, 0);
		if (ret < 0) {
			c->connected = 0;
		}
	}
	return ret;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void clean_ci_recv_thread (void *argp)
{
	ci_dev_t *c = (ci_dev_t *) argp;
	if (c->txdata) {
		free (c->txdata);
	}
	if (c->rxdata) {
		free (c->rxdata);
	}
	closesocket (c->fd_ci);
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void *ci_recv (void *argp)
{
	ci_dev_t *c = (ci_dev_t *) argp;
	ci_pdu_t tpdu;
	int ret = -1;

	pthread_cleanup_push (clean_ci_recv_thread, c);

	c->rxdata = (u_int8_t *) malloc (CA_TPDU_MAX + 2);
	if (!c->rxdata)
		err ("ci_recv: out of memory\n");
	c->txdata = (u_int8_t *) malloc (CA_TPDU_MAX + 2);
	if (!c->txdata)
		err ("ci_recv: out of memory\n");

	if (c->rxdata && c->txdata) {
		c->recv_run = 1;

		while (c->recv_run) {
			if (c->connected) {
				ret = recv (c->fd_ci, (char *) c->rxdata, CA_TPDU_MAX + 2, 0);
				if (ret > 0) {
					tpdu.data = c->rxdata;
					while (ret > 0) {
						tpdu.len = ntohs16 (tpdu.data);
						if (tpdu.len >= ret) {
							break;
						}
						tpdu.data += 2;
#ifdef SHOW_TPDU
						int j;
						info ("Received TPDU: ");
						for (j = 0; j < tpdu.len; j++) {
							info ("%02x ", tpdu.data[j]);
						}
						info ("\n");
#endif
						ci_decode_ll (tpdu.data, tpdu.len);
						unsigned int slot = (unsigned int) tpdu.data[0];
						if (slot < CA_MAX_SLOTS) {
							if (c->handle_ci_slot[slot]) {
								c->handle_ci_slot[slot] (&tpdu, c->handle_ci_slot_context[slot]);
							}
						}

						tpdu.data += tpdu.len;
						ret -= tpdu.len + 2;
					}
				} else {
					if (errno == EAGAIN) {
						ret = 0;
					} else {
						c->connected = 0;
					}
				}
			}
			usleep (10 * 1000);
		}
	}
	pthread_cleanup_pop (1);

	return NULL;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int ci_register_handler (ci_dev_t * c, int slot, int (*p) (ci_pdu_t *, void *), void *context)
{
	if (slot < CA_MAX_SLOTS) {
		c->handle_ci_slot[slot] = p;
		c->handle_ci_slot_context[slot] = context;
		return 0;
	}
	return -1;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int ci_unregister_handler (ci_dev_t * c, int slot)
{
	if (slot < CA_MAX_SLOTS) {
		c->handle_ci_slot[slot] = NULL;
		c->handle_ci_slot_context[slot] = NULL;
		return 0;
	}
	return -1;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static ci_dev_t *ci_add (void)
{
	ci_dev_t *c = (ci_dev_t *) malloc (sizeof (ci_dev_t));
	if (!c)
		return NULL;
	memset (c, 0, sizeof (ci_dev_t));
	dvbmc_list_add_head (&devs.list, &c->list);
	return c;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void ci_del (ci_dev_t * c)
{
	c->recv_run = 0;
	if (pthread_exist(c->ci_recv_thread) && !pthread_cancel (c->ci_recv_thread)) {
		pthread_join (c->ci_recv_thread, NULL);
	}
	dvbmc_list_remove (&c->list);
	free (c);
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


ci_dev_t *ci_find_dev_by_uuid (char *uuid)
{
	ci_dev_t *c;
	DVBMC_LIST_FOR_EACH_ENTRY (c, &devs.list, ci_dev_t, list) {
		if (!strcmp (c->uuid, uuid)) {
			return c;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static void *ci_handler (void *p)
{
	int n;
	netceiver_info_list_t *nc_list = nc_get_list ();
	ci_run = 1;
	while (ci_run) {
		nc_lock_list ();
		for (n = 0; n < nc_list->nci_num; n++) {
			netceiver_info_t *nci = nc_list->nci + n;
			char *uuid = nci->uuid;
			if (!strlen (uuid) || ci_find_dev_by_uuid (uuid)) {
				//already seen
				continue;
			}

			ci_dev_t *c = ci_add ();
			if (!c) {
				err ("Cannot get memory for dvb loopback context\n");
			}

			dbg ("allocate ci dev %d for uuid %s\n", dev_num, uuid);

			strcpy (c->uuid, uuid);
			c->cacaps = &nci->ci;
			c->device = dev_num++;

			info ("Starting ci thread for netceiver UUID %s\n", c->uuid);
			int ret = pthread_create (&c->ci_recv_thread, NULL, ci_recv, c);
			while (!ret && !c->recv_run) {
				usleep (10000);
			}
			if (ret) {
				err ("pthread_create failed with %d\n", ret);
			}
		}
		nc_unlock_list ();
		sleep (1);
	}
	return NULL;
}

int ci_init (int ca_enable, char *intf, int p)
{
	int ret = 0;
	if (intf) {
		strcpy (iface, intf);
	} else {
		iface[0] = 0;
	}
	if (p) {
		port = p;
	}

	dvbmc_list_init (&devs.list);
	if (ca_enable) {
		ret = pthread_create (&ci_handler_thread, NULL, ci_handler, NULL);
		while (!ret && !ci_run) {
			usleep (10000);
		}
	}
	return ret;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ci_exit (void)
{
	ci_dev_t *c;
	ci_dev_t *ctmp;
	if (pthread_exist (ci_handler_thread)) {
		if (pthread_exist(ci_handler_thread) && !pthread_cancel (ci_handler_thread)) {
			pthread_join (ci_handler_thread, NULL);
		}

		DVBMC_LIST_FOR_EACH_ENTRY_SAFE (c, ctmp, &devs.list, ci_dev_t, list) {
			ci_del (c);
		}
	}
}
