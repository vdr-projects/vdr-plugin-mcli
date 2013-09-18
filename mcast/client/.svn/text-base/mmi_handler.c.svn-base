/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 * modified by Reel Multimedia, http://www.reel-multimedia.com, info@reel-multimedia.com
 * 01042010 DL: use a single thread for reading from network layer (uses less resources)
 *
 */

#include "headers.h"

//---------------------------------------------------------------------------------------------
void mmi_print_info (mmi_info_t * m)
{
	char str[INET6_ADDRSTRLEN];
	printf ("------------------\n");
	inet_ntop (AF_INET6, &m->ipv6, (char *) str, INET6_ADDRSTRLEN);
	printf ("IP: %s\n", str);
	printf ("UUID: %s\n", m->uuid);
	printf ("Slot: %d\n", m->slot);

	int i;
	for (i = 0; i < m->caid_num; i++) {
		caid_mcg_t *cm = m->caids + i;
		printf ("%i.SID: %d\n", i, cm->caid);
		inet_ntop (AF_INET6, &cm->mcg, (char *) str, INET6_ADDRSTRLEN);
		printf ("%i.MCG: %s\n", i, str);
	}
	printf ("TEXT:\n===================\n %s \n===================\n", m->mmi_text);

}

//---------------------------------------------------------------------------------------------
int mmi_open_menu_session (char *uuid, char *intf, int port, int cmd)
{
	int ret;
	int j, sockfd;
	struct in6_addr ipv6;
	char iface[IFNAMSIZ];

	inet_pton (AF_INET6, uuid, &ipv6);

	if (!intf || !strlen (intf)) {
		struct intnode *intn = int_find_first ();
		if (intn) {
			strcpy (iface, intn->name);
		}
	} else {
		strncpy (iface, intf, sizeof (iface));
		iface[sizeof (iface) - 1] = 0;
	}
	if (!port) {
		port = 23013;
	}

	sockfd = socket (PF_INET6, SOCK_STREAM, 0);
	j = 1;
	if (setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, (_SOTYPE) & j, sizeof (j)) < 0) {
		err ("setsockopt REUSEADDR\n");
	}

	j = 1;
	if (setsockopt (sockfd, SOL_SOCKET, TCP_NODELAY, (_SOTYPE) & j, sizeof (j)) < 0) {
		warn ("setsockopt TCP_NODELAY\n");
	}

	dbg ("Connect To: %s\n", uuid);

	struct sockaddr_in6 addr;
	memset (&addr, 0, sizeof (struct sockaddr_in6));

	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons (port);
	addr.sin6_addr = ipv6;
	addr.sin6_scope_id = if_nametoindex (iface);

	ret = connect (sockfd, (struct sockaddr *) &addr, sizeof (struct sockaddr_in6));
	if (ret < 0) {
		dbg ("Failed to access NetCeiver MMI support\n");
		return -1;
	}
	//send init cmd
	char buf[128];
	memset (buf, 0, sizeof (buf));
	dbg ("Request CAM slot %d \n", cmd);
	sprintf (buf, "%x", cmd);
	int n = send (sockfd, buf, strlen (buf) + 1, 0);
	if (n < 0) {
		dbg ("unable to sent mmi connection cmd !\n");
		closesocket (sockfd);
		return -1;
	}
	dbg ("MMI SESSION : OK\n");
	return sockfd;
}

//---------------------------------------------------------------------------------------------
void mmi_close_menu_session (int s)
{
	closesocket (s);
}

//---------------------------------------------------------------------------------------------
int mmi_cam_reset (char *uuid, char *intf, int port, int slot)
{
	int cmd = (slot << 12) | 0xfff;
	printf ("Reseting slot %d (cmd %x)...\n", slot, cmd);
	int sock = mmi_open_menu_session (uuid, intf, port, cmd);
	if (sock < 1) {
		printf ("Unable to reset slot %d on netceiver %s...\n", slot, uuid);
	}
	closesocket (sock);
	return 0;
}
//---------------------------------------------------------------------------------------------
int mmi_cam_reinit (char *uuid, char *intf, int port, int slot)
{
	int cmd = (slot << 12) | 0xeee;
	printf ("Reinitializing slot %d (cmd %x)...\n", slot, cmd);
	int sock = mmi_open_menu_session (uuid, intf, port, cmd);
	if (sock < 1) {
		printf ("Unable to reset slot %d on netceiver %s...\n", slot, uuid);
	}
	closesocket (sock);
	return 0;
}
//---------------------------------------------------------------------------------------------
int mmi_get_menu_text (int sockfd, char *buf, int buf_len, int timeout)
{
	int n = -1;
	struct pollfd p;
	memset (buf, 0, buf_len);
	p.fd = sockfd;
	p.events = POLLIN;
	if (poll (&p, 1, (timeout+999)>>10) > 0) {
		n = recv (sockfd, buf, buf_len, 0);	//MSG_DONTWAIT);
	}
	if (n > 0) {
		dbg ("recv:\n%s \n", buf);
	}
	return n;
}

//---------------------------------------------------------------------------------------------
int mmi_send_menu_answer (int sockfd, char *buf, int buf_len)
{
	dbg ("send: %s len %d \n", buf, buf_len);
	int n;
	n = send (sockfd, buf, buf_len, 0);
	if (n < 0) {
		dbg ("mmi_send_answer: error sending !\n");
	}
	return n;
}

//---------------------------------------------------------------------------------------------
UDPContext *mmi_broadcast_client_init (int port, char *intf)
{
	UDPContext *s;
	char mcg[1024];
	char iface[IFNAMSIZ];
	//FIXME: move to common
	strcpy (mcg, "ff18:6000::");
	if (!intf || !strlen (intf)) {
		struct intnode *intn = int_find_first ();
		if (intn) {
			strcpy (iface, intn->name);
		}
	} else {
		strncpy (iface, intf, sizeof (iface));
		iface[sizeof (iface) - 1] = 0;
	}
	if (!port) {
		port = 23000;
	}

	s = client_udp_open_host (mcg, port, iface);
	if (!s) {
		dbg ("client udp open host error !\n");
	}

	return s;
}

void mmi_broadcast_client_exit (UDPContext * s)
{
	udp_close (s);
}

//---------------------------------------------------------------------------------------------
typedef struct
{
	xmlDocPtr doc;
	xmlChar *str, *key;
} xml_parser_context_t;

static void clean_xml_parser_thread (void *arg)
{
	xml_parser_context_t *c = (xml_parser_context_t *) arg;
	if (c->str) {
		xmlFree (c->str);
	}
	if (c->key) {
		xmlFree (c->key);
	}
	if (c->doc) {
		xmlFreeDoc (c->doc);
	}
	dbg ("Free XML parser structures!\n");
}

//---------------------------------------------------------------------------------------------
int mmi_get_data (xmlChar * xmlbuff, int buffersize, mmi_info_t * mmi_info)
{
	xml_parser_context_t c;
	xmlNode *root_element = NULL, *cur_node = NULL;

	xmlKeepBlanksDefault (0);	//reomve this f. "text" nodes
	c.doc = xmlParseMemory ((char *) xmlbuff, buffersize);
	root_element = xmlDocGetRootElement (c.doc);
	pthread_cleanup_push (clean_xml_parser_thread, &c);


	if (root_element != NULL) {
		cur_node = root_element->children;
		if (!xmlStrcmp (cur_node->name, (xmlChar *) "Description")) {
			root_element = cur_node->children;
			while (root_element != NULL) {
				c.key = NULL;
				c.str = NULL;
				if ((xmlStrcmp (root_element->name, (const xmlChar *) "component"))) {
					warn ("Cannot parse XML data\n");
					root_element = root_element->next;
					continue;
				}
				cur_node = root_element->children;
				if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Description"))) {
					c.str = xmlGetProp (cur_node, (unsigned char *) "about");
					//fprintf(stdout,"\n%s:\n",c.str);
					//fprintf(stdout,"-----------------------------------------------------------\n");
				} else {
					warn ("Cannot parse XML data\n");
					root_element = root_element->next;
					continue;
				}
				if (c.str && (!xmlStrcmp (c.str, (const xmlChar *) "MMIData"))) {
					cur_node = cur_node->children;
					while (cur_node != NULL) {
						if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "IP"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("IP: %s\n", c.key);
								inet_pton (AF_INET6, (char *) c.key, &mmi_info->ipv6);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "UUID"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("UUID: %s\n", c.key);
								strcpy (mmi_info->uuid, (char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Slot"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("Slot: %s\n", c.key);
								mmi_info->slot = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "TEXT"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("TEXT: %s\n", c.key);
								int olen = MMI_TEXT_LENGTH, ilen = strlen ((char *) c.key);

								UTF8Toisolat1 ((unsigned char *) mmi_info->mmi_text, &olen, c.key, &ilen);

								xmlFree (c.key);
							}
						}
						cur_node = cur_node->next;
					}
				} else if (c.str && (!xmlStrcmp (c.str, (const xmlChar *) "ProgramNumberIDs"))) {
					cur_node = cur_node->children;
					while (cur_node != NULL) {
						if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "MCG"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("MCG: %s\n", c.key);
								struct in6_addr mcg;
								inet_pton (AF_INET6, (char *) c.key, &mcg);
								int sid;
								mcg_get_id (&mcg, &sid);
								mcg_set_id (&mcg, 0);
								mmi_info->caids = (caid_mcg_t *) realloc (mmi_info->caids, sizeof (caid_mcg_t) * (mmi_info->caid_num + 1));
								if (!mmi_info->caids)
									err ("mmi_get_data: out of memory\n");
								caid_mcg_t *cm = mmi_info->caids + mmi_info->caid_num;
								cm->caid = sid;
								cm->mcg = mcg;
								mmi_info->caid_num++;
								xmlFree (c.key);
							}
						}
						cur_node = cur_node->next;
					}
				}
				xmlFree (c.str);
				root_element = root_element->next;
			}
		}
	}

	xmlFreeDoc (c.doc);
	pthread_cleanup_pop (0);
	return 1;
}

//---------------------------------------------------------------------------------------------
int mmi_poll_for_menu_text (UDPContext * s, mmi_info_t * m, int timeout)
{
	char buf[8192];
	int n = 0;
	if (s) {
		n = udp_read (s, (unsigned char *) buf, sizeof (buf), timeout, NULL);
		if (n > 0) {
			dbg ("recv:\n%s \n", buf);
			memset (m, 0, sizeof (mmi_info_t));
			mmi_get_data ((xmlChar *) buf, n, m);
		}
	}
	return n;
}
//---------------------------------------------------------------------------------------------
