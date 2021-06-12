/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#undef DEBUG
#include "headers.h"

extern int port;
extern char iface[IFNAMSIZ];

typedef struct
{
	xmlDocPtr doc;
	xmlChar *str, *key;
} xml_parser_context_t;

STATIC void clean_xml_parser_thread (void *arg)
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

int get_tra_data (xmlChar * xmlbuff, int buffersize, tra_info_t * tra_info)
{
	xml_parser_context_t c;
	xmlNode *root_element = NULL, *cur_node = NULL;

//	xmlKeepBlanksDefault (0);	//reomve this f. "text" nodes
//	c.doc = xmlParseMemory ((char *) xmlbuff, buffersize);
//	xmlKeepBlanksDefault doesn't work after patching cam flags
	c.doc = xmlReadMemory((char *) xmlbuff, buffersize, NULL, "UTF-8", XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NOBLANKS );
	root_element = xmlDocGetRootElement (c.doc);
	pthread_cleanup_push (clean_xml_parser_thread, &c);
	time_t t=time(NULL);

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
//                                      fprintf(stdout,"\n%s:\n",c.str);
//                                      fprintf(stdout,"-----------------------------------------------------------\n");
				} else {
					warn ("Cannot parse XML data\n");
					root_element = root_element->next;
					continue;
				}
#ifdef P2P
				if (c.str && (!xmlStrcmp (c.str, (const xmlChar *) "P2P_Data"))) {
					cur_node = cur_node->children;
					while (cur_node != NULL) {
						if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Quit"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("Quit: %s\n", c.key);
								tra_info->quit = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "TCA_ID"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("TCA_ID: %s\n", c.key);
								tra_info->tca_id = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "MC_Groups"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("MC_Groups: %s\n", c.key);
								tra_info->mca_grps = atoi ((char *) c.key);
								xmlFree (c.key);
							}						
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "IP"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("IP: %s\n", c.key);
								inet_pton (AF_INET6, (char *) c.key, &tra_info->ipv6);
								xmlFree (c.key);
							}
						}
						cur_node = cur_node->next;
					}
				} else if (c.str && (!xmlStrcmp (c.str, (const xmlChar *) "Tuner_Status"))) {
#else 
				if (c.str && (!xmlStrcmp (c.str, (const xmlChar *) "Tuner_Status"))) {
#endif
					cur_node = cur_node->children;
					tra_info->tra = (tra_t *) realloc (tra_info->tra, (tra_info->tra_num + 1) * sizeof (tra_t));
					if (!tra_info->tra) {
						err ("Cannot get memory for tra_t\n");
					}
					tra_t *tra = tra_info->tra + tra_info->tra_num;
					memset(tra, 0, sizeof (tra_t));
					tra->magic=MCLI_MAGIC;
					tra->version=MCLI_VERSION;
					
					while (cur_node != NULL) {
						if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Status"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("Status: %s\n", c.key);
								tra->s.st = (fe_status_t) atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Signal"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("Signal: %s\n", c.key);
								tra->s.strength = (u_int16_t) atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "SNR"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("SNR: %s\n", c.key);
								tra->s.snr = (u_int16_t) atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "BER"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("BER: %s\n", c.key);
								tra->s.ber = (u_int32_t) atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "UNC"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("UNC: %s\n", c.key);
								tra->s.ucblocks = (u_int32_t) atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Slot"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("Slot: %s\n", c.key);
								tra->slot = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "RotorStatus"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("Rotor: %s\n", c.key);
								tra->rotor_status = (u_int32_t) atoi ((char *) c.key);
								xmlFree (c.key);
							}

						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "RotorDiff"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("Rotor: %s\n", c.key);
								tra->rotor_diff = (u_int32_t) atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "UUID"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("UUID: %s\n", c.key);
								strncpy (tra->uuid, (char *) c.key, UUID_SIZE-1);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "MCG"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("MCG: %s\n", c.key);
								inet_pton (AF_INET6, (char *) c.key, &tra->mcg);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Redirect"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("Redirect: %s\n", c.key);
								tra->redirect = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "NIMCurrent"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("NIMCurrent: %s\n", c.key);
								tra->NIMCurrent = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "InUse"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("InUse: %s\n", c.key);
								tra->InUse = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Frequency"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("Frequency: %s\n", c.key);
								tra->fep.frequency = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Inversion"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("Inversion: %s\n", c.key);
								tra->fep.inversion = (fe_spectral_inversion_t)atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Type"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("Type: %s\n", c.key);
								tra->fe_type = (fe_type_t)atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} 
#ifdef P2P						
						else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Token"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("Token: %s\n", c.key);
								tra->token = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Preference"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("Preference: %s\n", c.key);
								tra->preference = atoi ((char *) c.key);
								xmlFree (c.key);
							}						
						} 
#endif						
						else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "SymbolRate"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("SymbolRate: %s\n", c.key);
								int val=atoi ((char *) c.key);
								switch((int)tra->fe_type) {
									case FE_DVBS2:
									case FE_QPSK:
										tra->fep.u.qpsk.symbol_rate=val;
										break;
									case FE_QAM:
										tra->fep.u.qam.symbol_rate=val;
										break;
									case FE_OFDM:
									default:
										break;
								}
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "FecInner"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("FecInner: %s\n", c.key);
								int val=atoi ((char *) c.key);
								switch((int)tra->fe_type) {
									case FE_DVBS2:
									case FE_QPSK:
										tra->fep.u.qpsk.fec_inner=(fe_code_rate_t)val;
										break;
									case FE_QAM:
										tra->fep.u.qam.fec_inner=(fe_code_rate_t)val;
										break;
									case FE_OFDM:
									default:
										break;
								}
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Modulation"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("Modulation: %s\n", c.key);
								int val=atoi ((char *) c.key);
								switch((int)tra->fe_type) {
									case FE_QAM:
										tra->fep.u.qam.modulation=(fe_modulation_t)val;
										break;
									case FE_ATSC:
										tra->fep.u.vsb.modulation=(fe_modulation_t)val;
										break;
									case FE_DVBS2:
									case FE_QPSK:
									case FE_OFDM:
									default:
										break;
										
								}
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Bandwidth"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("Bandwidth: %s\n", c.key);
								tra->fep.u.ofdm.bandwidth=(fe_bandwidth_t)atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "CodeRateHP"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("CodeRateHP: %s\n", c.key);
								tra->fep.u.ofdm.code_rate_HP=(fe_code_rate_t)atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "CodeRateLP"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("CodeRateLP: %s\n", c.key);
								tra->fep.u.ofdm.code_rate_LP=(fe_code_rate_t)atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Constellation"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("Constellation: %s\n", c.key);
								tra->fep.u.ofdm.constellation=(fe_modulation_t)atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "TransmissionMode"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("TransmissionMode: %s\n", c.key);
								tra->fep.u.ofdm.transmission_mode=(fe_transmit_mode_t)atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "GuardInterval"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("GuardInterval: %s\n", c.key);
								tra->fep.u.ofdm.guard_interval=(fe_guard_interval_t)atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "HierarchyInformation"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("HierarchyInformation: %s\n", c.key);
								tra->fep.u.ofdm.hierarchy_information=(fe_hierarchy_t)atoi ((char *) c.key);
								xmlFree (c.key);
							}
						}
						cur_node = cur_node->next;
					}
					tra->lastseen=t;
					tra_info->tra_num++;
				} else if (c.str && (!xmlStrcmp (c.str, (const xmlChar *) "CAM"))) {
					cur_node = cur_node->children;
					cam_info_t *cam = tra_info->cam + tra_info->cam_num;
					while(cur_node) {
						if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Slot"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								cam->slot = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Status"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								cam->status = atoi ((char *) c.key);								
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "MenuString"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								strncpy(cam->menu_string, (char *) c.key, MAX_MENU_STR_LEN-1);								
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Flags"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								cam->flags = (nc_ca_caps_t)atoi ((char *) c.key);								
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "MaxSids"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								cam->max_sids = atoi ((char *) c.key);								
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "UseSids"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								cam->use_sids = atoi ((char *) c.key);								
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "PmtFlag"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								cam->capmt_flag = atoi ((char *) c.key);								
								xmlFree (c.key);
							}
						}  						
						cur_node = cur_node->next;				
					}
					tra_info->cam_num++;	
				}
				xmlFree (c.str);
				root_element = root_element->next;
			}
		}
	}
	xmlFreeDoc (c.doc);
	pthread_cleanup_pop (0);
	return (1);
}

#ifdef CLIENT 
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef struct ccpp_thread_context
{
	UDPContext *s;
	xmlChar *buf;
	xmlChar *dst;
	int run;
} ccpp_thread_context_t;


//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
STATIC void clean_ccpp_thread (void *arg)
{
	ccpp_thread_context_t *c = (ccpp_thread_context_t *) arg;
	if (c->s) {
#ifdef	MULTI_THREAD_RECEIVER
		udp_close (c->s);
#else
		udp_close_buff (c->s);
#endif
	}
	if(c->buf) {
		free (c->buf);
	}
	if(c->dst) {
		free (c->dst);
	}
	dbg ("CCPP thread data buffer for tid %d freed !\n", gettid ());
}

void *recv_ten (void *arg)
{
	recv_info_t *r = (recv_info_t *) arg;
	ccpp_thread_context_t c;
	struct in6_addr ten = r->mcg;
	int n;
	tra_info_t tra_info;
	unsigned int dstlen;
	clock_t lastrecv=0;
	int donetimeout=0;

	memset (&c, 0, sizeof (ccpp_thread_context_t));
	pthread_cleanup_push (clean_ccpp_thread, &c);

	c.buf=(xmlChar *)malloc(XML_BUFLEN);
	if (!c.buf) {
		err ("Cannot get memory for TEN buffer\n");
	}
	c.dst=(xmlChar *)malloc(XML_BUFLEN * 5);
	if (!c.dst) {
		err ("Cannot get memory for TEN destination buffer\n");
	}
	
#ifdef FE_STATUS_CLEAR
	memset (&r->fe_status, 0, sizeof(recv_festatus_t));
	ioctl (r->fd, DVBLO_SET_FRONTEND_STATUS, &r->fe_status);
#endif	
	memset (&tra_info, 0, sizeof (tra_info_t));
	tra_info.magic=MCLI_MAGIC;
	tra_info.version=MCLI_VERSION;
	
	mcg_set_streaming_group (&ten, STREAMING_TEN);
#ifdef MULTI_THREAD_RECEIVER
	c.s = client_udp_open (&ten, port, iface);
#else	
	c.s = client_udp_open_buff (&ten, port, iface, XML_BUFLEN);
#endif
	if (!c.s) {
		warn ("%s: client_udp_open error on interface=%s\n", __FUNCTION__, iface);
	} else {
#ifdef DEBUG	
		char host[INET6_ADDRSTRLEN];
		inet_ntop (AF_INET6, &ten, (char *) host, INET6_ADDRSTRLEN);
		dbg ("Start receive TEN for tid %d receiver %p at %s port %d %s\n", gettid (), r, host, port, iface);
#endif
		r->ten_run = 1;
		while (r->ten_run) {
#ifdef MULTI_THREAD_RECEIVER
			if ((n = udp_read (c.s, c.buf, XML_BUFLEN, 1000, NULL)) > 0) {
#else
			usleep(100000); // 10 times per seconds should be enough
			if ((n = udp_read_buff (c.s, c.buf, XML_BUFLEN, 1000, NULL)) > 0) {
#endif
				dstlen = XML_BUFLEN*5;
				if (!gunzip (c.dst, &dstlen, c.buf, n)) {
					memset (&tra_info, 0, sizeof (tra_info_t));
					tra_info.magic=MCLI_MAGIC;
					tra_info.version=MCLI_VERSION;
					
					pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL);
					if (get_tra_data (c.dst, dstlen, &tra_info)) {
						lastrecv=clock();
						donetimeout=0;
						if (tra_info.tra_num) {
							r->fe_status = tra_info.tra->s;
							if(r->handle_ten) {
								r->handle_ten (tra_info.tra, r->handle_ten_context);
							}

							if (tra_info.tra->redirect) {
#ifdef DEBUG							
								char hostname[INET6_ADDRSTRLEN];
								inet_ntop (AF_INET6, &tra_info.tra->mcg, hostname, INET6_ADDRSTRLEN);
								dbg ("Redirect for receiver %p: MCG is at %s\n", r, hostname);
#endif								
								int ret = recv_redirect (r, tra_info.tra->mcg);

								if (ret) {
									printf("New MCG for recv_ten !\n");
#ifdef	MULTI_THREAD_RECEIVER
									udp_close (c.s);
#else
									udp_close_buff (c.s);
#endif
									struct in6_addr ten = r->mcg;
									mcg_set_streaming_group (&ten, STREAMING_TEN);
#ifdef MULTI_THREAD_RECEIVER
									c.s = client_udp_open (&ten, port, iface);
#else	
									c.s = client_udp_open_buff (&ten, port, iface, XML_BUFLEN);
#endif
									if (!c.s) {
										warn ("%s: client_udp_open error on interface=%s\n", __FUNCTION__, iface);
										break;
									}
								}
							}
						}
						free (tra_info.tra);
						tra_info.tra=NULL;
					}
					pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
				} else {
					dbg ("uncompress failed\n");
				}
			} else {
				if (!donetimeout && (clock()-lastrecv)>(TEN_TIMEOUT*CLOCKS_PER_SEC)) {
					donetimeout=1;
					memset (&r->fe_status, 0, sizeof(recv_festatus_t));
					if(r->handle_ten) {
						r->handle_ten (NULL, r->handle_ten_context);
					}
					dbg ("Signal Timeout on receiver %p!\n", r);
				}
			}
			pthread_testcancel();
		}
#ifdef DEBUG		
		dbg ("Stop receive TEN on receiver %p %s %d %s\n", r, host, port, iface);
#endif
	}
	pthread_cleanup_pop (1);
	r->ten_run = 1;
	return NULL;
}

int register_ten_handler (recv_info_t * r, int (*p) (tra_t *, void *c), void *c)
{
	r->handle_ten=p;
	r->handle_ten_context=c;
	return 0;
}
                
void *recv_tra (void *arg)
{
	ccpp_thread_context_t c;
	int n;
	tra_info_t tra_info;
	unsigned int dstlen;
	struct in6_addr tra;

	memset (&c, 0, sizeof (ccpp_thread_context_t));
	pthread_cleanup_push (clean_ccpp_thread, &c);

	c.buf=(xmlChar *)malloc(XML_BUFLEN);
	if (!c.buf) {
		err ("Cannot get memory for TRA buffer\n");
	}
	c.dst=(xmlChar *)malloc(XML_BUFLEN * 5);
	if (!c.dst) {
		err ("Cannot get memory for TRA destination buffer\n");
	}

	mcg_init_streaming_group (&tra, STREAMING_TRA);
	
#ifdef MULTI_THREAD_RECEIVER
	c.s = client_udp_open (&tra, port, iface);
#else	
	c.s = client_udp_open_buff (&tra, port, iface, XML_BUFLEN);
#endif
	if (!c.s) {
		warn ("%s: client_udp_open error on interface=%s\n", __FUNCTION__, iface);
	} else {
		c.run=1;
#ifdef DEBUG
		char host[INET6_ADDRSTRLEN];
		inet_ntop (AF_INET6, &tra, (char *) host, INET6_ADDRSTRLEN);
		dbg ("Start receive TRA at %s port %d %s\n",  host, port, iface);
#endif
		while (c.run) {
#ifdef MULTI_THREAD_RECEIVER
			if ((n = udp_read (c.s, c.buf, XML_BUFLEN, 500000, NULL)) > 0) {
#else
			usleep(100000); // 10 times per seconds should be enough
			if ((n = udp_read_buff (c.s, c.buf, XML_BUFLEN, 500000, NULL)) > 0) {
#endif
				dstlen = XML_BUFLEN*5;
				if (!gunzip (c.dst, &dstlen, c.buf, n)) {
					memset (&tra_info, 0, sizeof (tra_info_t));
					tra_info.magic=MCLI_MAGIC;
					tra_info.version=MCLI_VERSION;
				
					pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL);
					if (get_tra_data (c.dst, dstlen, &tra_info)) {
						handle_tra (&tra_info);
					}
					pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
					} else {
					dbg ("uncompress failed\n");
				}
			} 
#ifdef DEBUG
			dbg ("Stop receive TRA on %s %d %s len:%d\n", host, port, iface, n);
#endif
			pthread_testcancel();
		}
	}
	pthread_cleanup_pop (1);
	return NULL;
}
#endif

//--------------------------------------------------------------------------------------------------------------------------
int get_tca_data (xmlChar * xmlbuff, int buffersize, netceiver_info_t * nc_info)
{
	xml_parser_context_t c;
	xmlNode *root_element, *cur_node;

//	xmlKeepBlanksDefault (0);	//reomve this f. "text" nodes
//	c.doc = xmlParseMemory ((char *) xmlbuff, buffersize);
//	xmlKeepBlanksDefault doesn't work after patching cam flags
	c.doc = xmlReadMemory((char *) xmlbuff, buffersize, NULL, "UTF-8", XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NOBLANKS );
	root_element = xmlDocGetRootElement (c.doc);
	pthread_cleanup_push (clean_xml_parser_thread, &c);
	nc_info->magic=MCLI_MAGIC;
	nc_info->version=MCLI_VERSION;

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
				} else {
					warn ("Cannot parse XML data\n");
					root_element = root_element->next;
					continue;
				}
				if (c.str && (!xmlStrcmp (c.str, (const xmlChar *) "Platform"))) {
					cur_node = cur_node->children;
					while (cur_node != NULL) {
						if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "OSVersion"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("OSVersion: %s\n", c.key);
								strncpy (nc_info->OSVersion, (char *) c.key, UUID_SIZE-1);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "AppVersion"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("AppVersion: %s\n", c.key);
								strncpy (nc_info->AppVersion, (char *) c.key, UUID_SIZE-1);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "FirmwareVersion"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("FirmwareVersion: %s\n", c.key);
								strncpy (nc_info->FirmwareVersion, (char *) c.key, UUID_SIZE-1);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "HardwareVersion"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("HardwareVersion: %s\n", c.key);
								strncpy (nc_info->HardwareVersion, (char *) c.key, UUID_SIZE-1);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Serial"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("Serial: %s\n", c.key);
								strncpy (nc_info->Serial, (char *) c.key, UUID_SIZE-1);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Vendor"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("Vendor: %s\n", c.key);
								strncpy (nc_info->Vendor, (char *) c.key, UUID_SIZE-1);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "DefCon"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("DefCon: %s\n", c.key);
								nc_info->DefCon = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "UUID"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("UUID: %s\n", c.key);
								strncpy (nc_info->uuid, (char *) c.key, UUID_SIZE-1);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Description"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("Description: %s\n", c.key);
								strncpy (nc_info->Description, (char *) c.key, UUID_SIZE-1);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "IP"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("IP: %s\n", c.key);
								inet_pton (AF_INET6, (char *) c.key, &nc_info->ip);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "ProcessUptime"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("ProcessUptime: %s\n", c.key);
								nc_info->ProcessUptime=atoi((char *)c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "SystemUptime"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("SystemUptime: %s\n", c.key);
								nc_info->SystemUptime=atoi((char *)c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "TunerTimeout"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("TunerTimeout: %s\n", c.key);
								nc_info->TunerTimeout=atoi((char *)c.key);
								xmlFree (c.key);
							}
						}
						cur_node = cur_node->next;
					}
				} else if (c.str && (!xmlStrcmp (c.str, (const xmlChar *) "Tuner"))) {
					cur_node = cur_node->children;
					nc_info->tuner = (tuner_info_t *) realloc (nc_info->tuner, (nc_info->tuner_num + 1) * sizeof (tuner_info_t));
					if (!nc_info->tuner) {
						err ("Cannot get memory for tuner_info\n");
					}

					tuner_info_t *t = nc_info->tuner + nc_info->tuner_num;
					memset (t, 0, sizeof (tuner_info_t));
					t->magic=MCLI_MAGIC;
					t->version=MCLI_VERSION;

					while (cur_node != NULL) {
						if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Name"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								strncpy (t->fe_info.name, (char *) c.key, 127);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Type"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								if ((!xmlStrcmp (c.key, (const xmlChar *) "DVB-S")))
									t->fe_info.type = FE_QPSK;
								if ((!xmlStrcmp (c.key, (const xmlChar *) "DVB-S2")))
									t->fe_info.type = (fe_type_t)FE_DVBS2;
								if ((!xmlStrcmp (c.key, (const xmlChar *) "DVB-C")))
									t->fe_info.type = FE_QAM;
								if ((!xmlStrcmp (c.key, (const xmlChar *) "DVB-T")))
									t->fe_info.type = FE_OFDM;
								if ((!xmlStrcmp (c.key, (const xmlChar *) "ATSC")))
									t->fe_info.type = FE_ATSC;
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "FrequencyMin"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								t->fe_info.frequency_min = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "FrequencyMax"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								t->fe_info.frequency_max = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "FrequencyStepSize"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								t->fe_info.frequency_stepsize = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "FrequencyTolerance"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								t->fe_info.frequency_tolerance = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "SymbolRateMin"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								t->fe_info.symbol_rate_min = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "SymbolRateMax"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								t->fe_info.symbol_rate_max = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "SymbolRateTolerance"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								t->fe_info.symbol_rate_tolerance = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Caps"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								t->fe_info.caps = (fe_caps_t)atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Slot"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								t->slot = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Preference"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								t->preference = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "UUID"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								strncpy (t->uuid, (char *) c.key, UUID_SIZE-1);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "SatelliteListName"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								strncpy (t->SatelliteListName, (char *) c.key, UUID_SIZE-1);
								xmlFree (c.key);
							}
						}
						cur_node = cur_node->next;
					}
					nc_info->tuner_num++;
				} else if (c.str && !(xmlStrcmp (c.str, (xmlChar *) "CI"))) {
					cur_node = cur_node->children;
					recv_cacaps_t *ci=&nc_info->ci;
					while(cur_node) {
						if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "component"))) {
							xmlNode *l2_node = cur_node->children;
							if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "Description"))) {
								c.key = xmlGetProp (l2_node, (unsigned char *) "about");
								dbg ("Parsing CI-Description: %s\n", c.key);
								if (c.key && !xmlStrcmp (c.key, (xmlChar *) "Capabilities")) {
									xmlFree (c.key);
									xmlNode * l3_node = l2_node->children;
									while (l3_node != NULL) {
										dbg ("Capability-Element: %s\n", l3_node->name);
										if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "SlotNum"))) {
											c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
											if(c.key) {
												dbg ("SlotNum: %s\n", c.key);
												ci->cap.slot_num=atoi((char*)c.key);
												xmlFree (c.key);
											}
										} else if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "SlotType"))) {
											c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
											if(c.key) {
												dbg ("SlotType: %s\n", c.key);
												ci->cap.slot_type=atoi((char*)c.key);
												xmlFree (c.key);
											}
										} else if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "DescrNum"))) {
											c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
											if(c.key) {
												dbg ("DescrNum: %s\n", c.key);
												ci->cap.descr_num=atoi((char*)c.key);
												xmlFree (c.key);
											}
										} else if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "DescrType"))) {
											c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
											if(c.key) {
												dbg ("DescrType: %s\n", c.key);
												ci->cap.descr_type=atoi((char*)c.key);
												xmlFree (c.key);
											}
										}
										l3_node = l3_node->next;
									}
								} else if (c.key && !xmlStrcmp (c.key, (xmlChar *) "Slot")) {
									xmlFree (c.key);
									xmlNode *l3_node = l2_node->children;
									int slot=-1;
										while (l3_node != NULL) {
										dbg ("Slot-Element: %s\n", l3_node->name);
										if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "Num"))) {
											c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
											if(c.key) {
												dbg ("Num: %s\n", c.key);
												int x=atoi((char*)c.key);
												if( (x < 0) || (x >= CA_MAX_SLOTS) ) {
													continue;
												}
												slot=x;
												ci->info[slot].num=slot;
												xmlFree (c.key);
											}
										} else if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "Type"))) {
											c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
											if(c.key) {
												dbg ("Type: %s\n", c.key);
												if(slot>=0) {
													ci->info[slot].type=atoi((char*)c.key);
												}
												xmlFree (c.key);
											}
										} else if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "Flags"))) {
											c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
											if(c.key) {
												dbg ("Flags: %s\n", c.key);
												if(slot>=0) {
													ci->info[slot].flags=atoi((char*)c.key);
												}
												xmlFree (c.key);
											}
										}
										l3_node = l3_node->next;
									}
								}
							}
						}
						cur_node = cur_node->next;
					}			
				//CAM start
				} else if (c.str && !(xmlStrcmp (c.str, (xmlChar *) "CAM"))) {
					cur_node = cur_node->children;
					cam_info_t *cam = nc_info->cam + nc_info->cam_num;
					while(cur_node) {
						if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Slot"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								cam->slot = atoi ((char *) c.key);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Status"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								cam->status = atoi ((char *) c.key);								
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "MenuString"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								strncpy(cam->menu_string, (char *) c.key, MAX_MENU_STR_LEN-1);								
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Flags"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								cam->flags = (nc_ca_caps_t)atoi ((char *) c.key);								
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "MaxSids"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								cam->max_sids = atoi ((char *) c.key);								
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "UseSids"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								cam->use_sids = atoi ((char *) c.key);								
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "PmtFlag"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								cam->capmt_flag = atoi ((char *) c.key);								
								xmlFree (c.key);
							}
						}  						
						cur_node = cur_node->next;
					}			
					nc_info->cam_num++;						
					//CAM end
				} else if (c.str && !(xmlStrcmp (c.str, (xmlChar *) "SatelliteList"))) {
					cur_node = cur_node->children;
					nc_info->sat_list = (satellite_list_t *) realloc (nc_info->sat_list, (nc_info->sat_list_num + 1) * sizeof (satellite_list_t));
					if (!nc_info->sat_list) {
						err ("Cannot get memory for sat_list\n");
					}

					satellite_list_t *sat_list = nc_info->sat_list + nc_info->sat_list_num;
					memset (sat_list, 0, sizeof (satellite_list_t));
					sat_list->magic=MCLI_MAGIC;
					sat_list->version=MCLI_VERSION;

					while (cur_node != NULL) {
						if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "SatelliteListName"))) {
							c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
							if (c.key) {
								dbg ("SatelliteListName: %s\n", c.key);
								strncpy (sat_list->Name, (char *) c.key, UUID_SIZE-1);
								xmlFree (c.key);
							}
						} else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "component"))) {
							xmlNode *l2_node = cur_node->children;
							if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "Description"))) {
								c.key = xmlGetProp (l2_node, (unsigned char *) "about");
								dbg ("Parsing L2-Description: %s\n", c.key);
								if (c.key && !xmlStrcmp (c.key, (xmlChar *) "Satellite")) {
									xmlFree (c.key);
									l2_node = l2_node->children;
									sat_list->sat = (satellite_info_t *) realloc (sat_list->sat, (sat_list->sat_num + 1) * sizeof (satellite_info_t));
									if (!sat_list->sat) {
										err ("Cannot get memory for sat\n");
									}

									satellite_info_t *sat = sat_list->sat + sat_list->sat_num;
									memset (sat, 0, sizeof (satellite_info_t));
									sat->magic=MCLI_MAGIC;
									sat->version=MCLI_VERSION;

									while (l2_node != NULL) {
										dbg ("L2-Element: %s\n", l2_node->name);
										if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "Name"))) {
											c.key = xmlNodeListGetString (c.doc, l2_node->xmlChildrenNode, 1);
											if(c.key) {
												dbg ("Name: %s\n", c.key);
												strncpy (sat->Name, (char *) c.key, UUID_SIZE-1);
												xmlFree (c.key);
											}
										} else if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "Position"))) {
											c.key = xmlNodeListGetString (c.doc, l2_node->xmlChildrenNode, 1);
											if(c.key) {
												dbg ("Position: %s\n", c.key);
												sat->SatPos = atoi ((char *) c.key);
												xmlFree (c.key);
											}
										} else if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "PositionMin"))) {
											c.key = xmlNodeListGetString (c.doc, l2_node->xmlChildrenNode, 1);
											if(c.key) {
												dbg ("PositionMin: %s\n", c.key);
												sat->SatPosMin = atoi ((char *) c.key);
												xmlFree (c.key);
											}
										} else if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "PositionMax"))) {
											c.key = xmlNodeListGetString (c.doc, l2_node->xmlChildrenNode, 1);
											if(c.key) {
												dbg ("PositionMax: %s\n", c.key);
												sat->SatPosMax = atoi ((char *) c.key);
												xmlFree (c.key);
											}
										} else if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "AutoFocus"))) {
											c.key = xmlNodeListGetString (c.doc, l2_node->xmlChildrenNode, 1);
											if(c.key) {
												dbg ("AutoFocus: %s\n", c.key);
												sat->AutoFocus = atoi ((char *) c.key);
												xmlFree (c.key);
											}
										} else if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "Latitude"))) {
											c.key = xmlNodeListGetString (c.doc, l2_node->xmlChildrenNode, 1);
											if(c.key) {
												dbg ("Latitude: %s\n", c.key);
												sat->Latitude = atoi ((char *) c.key);
												xmlFree (c.key);
											}
										} else if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "Longitude"))) {
											c.key = xmlNodeListGetString (c.doc, l2_node->xmlChildrenNode, 1);
											if(c.key) {
												dbg ("Longitude: %s\n", c.key);
												sat->Longitude = atoi ((char *) c.key);
												xmlFree (c.key);
											}
										} else if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "Type"))) {
											c.key = xmlNodeListGetString (c.doc, l2_node->xmlChildrenNode, 1);
											if(c.key) {
												dbg ("Type: %s\n", c.key);
												sat->type = (satellite_source_t)atoi ((char *) c.key);
												xmlFree (c.key);
											}
										} else if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "component"))) {
											xmlNode *l3_node = l2_node->children;

											if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "Description"))) {
												c.key = xmlGetProp (l3_node, (unsigned char *) "about");
												dbg ("Parsing L3-Description: %s\n", c.key);
												if (c.key && !xmlStrcmp (c.key, (xmlChar *) "SatelliteComponent")) {
													xmlFree (c.key);
													l3_node = l3_node->children;
													dbg ("Now checking for SatelliteCompontents\n");
													sat->comp = (satellite_component_t *) realloc (sat->comp, (sat->comp_num + 1) * sizeof (satellite_component_t));
													if (!sat->comp) {
														err ("Cannot get memory for comp\n");
													}

													satellite_component_t *comp = sat->comp + sat->comp_num;
													memset (comp, 0, sizeof (satellite_component_t));
													comp->magic=MCLI_MAGIC;
													comp->version=MCLI_VERSION;

													while (l3_node != NULL) {
														dbg ("L3-Element: %s\n", l3_node->name);
														if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "Polarisation"))) {
															c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
															if(c.key) {
																dbg ("Polarisation: %s\n", c.key);
																comp->Polarisation = (polarisation_t)atoi ((char *) c.key);
																xmlFree (c.key);
															}
														} else if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "RangeMin"))) {
															c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
															if(c.key) {
																dbg ("RangeMin: %s\n", c.key);
																comp->RangeMin = atoi ((char *) c.key);
																xmlFree (c.key);
															}
														} else if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "RangeMax"))) {
															c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
															if(c.key) {
																dbg ("RangeMax: %s\n", c.key);
																comp->RangeMax = atoi ((char *) c.key);
																xmlFree (c.key);
															}
														} else if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "LOF"))) {
															c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
															if(c.key) {
																dbg ("LOF: %s\n", c.key);
																comp->LOF = atoi ((char *) c.key);
																xmlFree (c.key);
															}
														} else if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "Voltage"))) {
															c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
															if(c.key) {
																dbg ("Voltage: %s\n", c.key);
																comp->sec.voltage = (fe_sec_voltage_t)atoi ((char *) c.key);
																xmlFree (c.key);
															}
														} else if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "Tone"))) {
															c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
															if(c.key) {
																dbg ("Tone: %s\n", c.key);
																comp->sec.tone_mode = (fe_sec_tone_mode_t)atoi ((char *) c.key);
																xmlFree (c.key);
															}
														} else if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "MiniCmd"))) {
															c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
															if(c.key) {
																dbg ("MiniCmd: %s\n", c.key);
																comp->sec.mini_cmd = (fe_sec_mini_cmd_t)atoi ((char *) c.key);
																xmlFree (c.key);
															}
														} else if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "DiSEqC_Cmd"))) {
															c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
															dbg ("DiSEqC_Cmd: %s\n", c.key);
															if(c.key) {
																int v[6], i, n=0;
																char *s= (char *)c.key;
																struct dvb_diseqc_master_cmd *diseqc_cmd=&comp->sec.diseqc_cmd;
																do {
																	dbg("Parsing: %s\n",s);
																	diseqc_cmd->msg_len = sscanf (s, "%x %x %x %x %x %x", v, v + 1, v + 2, v + 3, v + 4, v + 5);
																	for (i = 0; i < diseqc_cmd->msg_len; i++) {
																		diseqc_cmd->msg[i] = v[i];
																	}
																	s=strchr(s,',');
																	if(s) {
																		s++;
																	}
																	diseqc_cmd=comp->diseqc_cmd+n;
																	n++;
																} while(s && n<=DISEQC_MAX_EXTRA);
																xmlFree (c.key);
																comp->diseqc_cmd_num=n;
															}
														}
														l3_node = l3_node->next;
													}
													sat->comp_num++;
												} else {
													xmlFree (c.key);
												}
											}
										}
										l2_node = l2_node->next;
									}
									sat_list->sat_num++;
								} else {
									xmlFree (c.key);
								}
							}
						}
						cur_node = cur_node->next;
					}
					nc_info->sat_list_num++;
				}
				xmlFree (c.str);
				root_element = root_element->next;
			}
		}
	}

	xmlFreeDoc (c.doc);
	pthread_cleanup_pop (0);
	return (1);
}

#ifdef CLIENT

void *recv_tca (void *arg)
{
	int n;
	ccpp_thread_context_t c;
	unsigned int dstlen;
	netceiver_info_t nc_info;
	struct in6_addr tca;

	memset (&c, 0, sizeof (ccpp_thread_context_t));
	pthread_cleanup_push (clean_ccpp_thread, &c);

	c.buf=(xmlChar *)malloc(XML_BUFLEN);
	if (!c.buf) {
		err ("Cannot get memory for TRA buffer\n");
	}
	c.dst=(xmlChar *)malloc(XML_BUFLEN * 5);
	if (!c.dst) {
		err ("Cannot get memory for TRA destination buffer\n");
	}
	
	mcg_init_streaming_group (&tca, STREAMING_TCA);
	
#ifdef MULTI_THREAD_RECEIVER
	c.s = client_udp_open (&tca, port, iface);
#else	
	c.s = client_udp_open_buff (&tca, port, iface, XML_BUFLEN);
#endif
	if (!c.s) {
		warn ("%s: client_udp_open error on interface=%s\n", __FUNCTION__, iface);
	} else {
		c.run=1;
#ifdef DEBUG
		char host[INET6_ADDRSTRLEN];
		inet_ntop (AF_INET6, &tca, (char *) host, INET6_ADDRSTRLEN);
		dbg ("Start Receive TCA on interface %s port %d\n", iface, port);
#endif
		while (c.run) {
#ifdef MULTI_THREAD_RECEIVER
			if ((n = udp_read (c.s, c.buf, XML_BUFLEN, 500000, NULL)) > 0) {
#else			
			usleep(100000); // 10 times per seconds should be enough
			if ((n = udp_read_buff (c.s, c.buf, XML_BUFLEN, 500000, NULL)) > 0) {
#endif
				dstlen = XML_BUFLEN * 5;
				if (!gunzip (c.dst, &dstlen, c.buf, n)) {
					memset (&nc_info, 0, sizeof (netceiver_info_t));
				
					pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL);
					get_tca_data (c.dst, dstlen, &nc_info);
					handle_tca (&nc_info);
					pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
				} else {
					dbg ("uncompress failed\n");
				}
			}
			pthread_testcancel();		
		}
	}
	pthread_cleanup_pop (1);
	return NULL;
}
#endif
