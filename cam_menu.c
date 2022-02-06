/***************************************************************************
 *   Copyright (C) 2009;   Author:  Tobias Bratfisch                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *
 ***************************************************************************/


#include "cam_menu.h"
#include "filter.h"
#include "device.h"

#include <vdr/plugin.h>
#include <string.h>

#define TMP_PATH "/tmp"
#define TMP_FILE TMP_PATH"/netceiver.conf"

extern bool m_netcvupdate_use_lftp;
extern bool m_netcvupdate_enable_debug;

class cCamMtd : public cMenuEditIntItem {
	friend class cNCUpdate;
	public:
		cCamMtd(const char *uuid, int slot, const char *info, nc_ca_caps_t flags) : 
				cMenuEditIntItem((const char *)cString::sprintf("      %s", tr("Multi-Transponder")), &m_flags, 1, 2, trVDR("no"), trVDR("yes")) {
			m_oflags = m_flags = flags;
			if(uuid) strcpy (m_uuid, uuid); else m_uuid[0]=0;
			if(info) strcpy (m_info, info); else m_info[0]=0;
			m_slot = slot;
			Set();
		}; // cCamMtd
		virtual bool MtdModified() { return m_flags != m_oflags; };
		virtual void MtdSaved()    { m_oflags = m_flags; }
	protected:
		char m_uuid[UUID_SIZE];
		int m_slot;
		char m_info[MMI_TEXT_LENGTH];
		int m_flags;
		int m_oflags;

}; // cCamMtd

class cCamInfo : public cOsdItem {
	public:
		cCamInfo(const char *uuid, int slot, const char *info) {
			SetText(cString::sprintf ("   %s:\t%s", slot == 0 ? trVDR ("lower slot") : trVDR ("upper slot"), 
				                                      info ? info[0] ? info : trVDR ("Error") : trVDR ("No CI-Module")));
			if(uuid) strcpy (m_uuid, uuid); else m_uuid[0]=0;
			if(info) strcpy (m_info, info); else m_info[0]=0;
			m_hascam = (info!=NULL);
			m_slot = slot;
		}; // cCamInfo
		int CamMenuOpen (char *iface) {
			isyslog("mcli::%s: Opening CAM Menu at NetCeiver %s Slot %d info %s", __FUNCTION__, m_uuid, m_slot, m_info);
			int mmi_session = 0;
			if(m_slot != -1 && strlen(m_info)) {
				mmi_session = mmi_open_menu_session (m_uuid, iface, 0, m_slot);
				if (mmi_session > 0) {
					sleep (1);
					const char *c = "00000000000000\n";
					mmi_send_menu_answer (mmi_session, (char *) c, strlen (c));
				} else {
					esyslog("Could not open mcli menu session for %s/%d(%s): %d", m_uuid, m_slot, m_info, mmi_session);
					mmi_session = -1;
				} // if
			} // if
			return mmi_session;
		}; // CamMenuOpen
		virtual void CamReset (char *iface) {
			if(CanCamReset())
				mmi_cam_reset(m_uuid, iface, 0, m_slot);
		}; // CamMenuOpen
		virtual bool CanCamReset() {
			return (m_slot != -1) && m_hascam;
		}; // CanCamReset
		virtual bool MtdPossible() {
			return MtdPossible(m_info);
		}; // MtdPossible
		static bool MtdPossible(const char *info) {
			if(info && strlen(info))
				if (strncmp(info, "Alpha", 5) == 0 
                                    || strncmp(info, "easy.TV", 7) == 0
                                    || strncmp(info, "Power", 5) == 0
                                   )
					return true;
			return false;
		}; // MtdPossible
	protected:
		bool m_hascam;
		char m_uuid[UUID_SIZE];
		int m_slot;
		char m_info[MMI_TEXT_LENGTH];
}; // cCamInfo

class cNCUpdate : public cThread {
	public:
		cNCUpdate(cCamMenu *menu, const char *iface) {
			m_menu = menu;
			m_iface = iface;
			if(m_iface && !strlen(m_iface)) m_iface = NULL;
			m_state=1;
			Start();
		}; // cNCUpdate
		virtual bool Done() { return !m_state; };
		virtual const char *GetStateStr() {
			return m_statestr;
		}; // GetStateStr
	protected:
		virtual void Action() {
			char uuid[UUID_SIZE];
			while(Running() && m_menu && m_state) {
				switch(m_state) {
					case 1: {
						m_statestr=tr("Updating configuration...");
						m_state = 0;
						for(cOsdItem *i=m_menu->First(); i; i=m_menu->Next(i)) {
							cCamMtd *m = dynamic_cast<cCamMtd *>(i);
							if(m && m->MtdModified()) {
								strcpy(uuid, m->m_uuid);
								m_state = 2;
								break;
							} // if
						} // for
						if(!m_state) m_statestr=tr("Configuration is up to date...");
						break;
					} // case 1
					case 2: {
						m_statestr = cString::sprintf(tr("Getting configuration from Netceiver %s"), uuid);
						cString c = cString::sprintf("rm -f %s; cd %s; netcvupdate%s%s -i %s%s%s -D"
							, TMP_FILE
							, TMP_PATH
							, m_netcvupdate_use_lftp ? " -n" : ""
							, m_netcvupdate_enable_debug ? " -e" : ""
							, uuid
							, m_iface ? " -d " : ""
							, m_iface ? m_iface : ""
							);
						dsyslog("EXEC1 %s", (const char *)c);
						if(SystemExec(c)) {
							m_statestr = cString::sprintf(tr("Failed to get configuration from Netceiver %s"), uuid);
							m_state = 0;
							continue;
						} // if
						m_state = 3;
						break;
					} // case 2
					case 3: {
						m_statestr = cString::sprintf(tr("Changing configuration for Netceiver %s"), uuid);
						xmlDocPtr doc = xmlReadFile(TMP_FILE, NULL, 0);
						if(doc == NULL) {
							m_statestr = cString::sprintf(tr("Failed to parse configuration from Netceiver %s"), uuid);
							m_state = 0;
							continue;
						} // if
						if(!PatchAllCamFlags(doc, uuid)) {
							xmlFreeDoc(doc);
							m_statestr = cString::sprintf(tr("Failed to set configuration for Netceiver %s"), uuid);
							m_state = 0;
							continue;
						} // if
						if(xmlSaveFormatFileEnc(TMP_FILE, doc, "UTF-8", 1)==-1) {
							xmlFreeDoc(doc);
							m_statestr = cString::sprintf(tr("Failed to save configuration for Netceiver %s"), uuid);
							m_state = 0;
							continue;
						} // if
						xmlFreeDoc(doc);
						m_state=4;
						break;
					} // case 3
					case 4: { 
						m_statestr = cString::sprintf(tr("Saving configuration for Netceiver %s"), uuid);
						cString c = cString::sprintf("netcvupdate%s%s -i %s%s%s -U %s -K"
							, m_netcvupdate_use_lftp ? " -n" : ""
							, m_netcvupdate_enable_debug ? " -e" : ""
							, uuid
							, m_iface ? " -d " : ""
							, m_iface ? m_iface : ""
							, TMP_FILE
							);
						dsyslog("EXEC2 %s", (const char *)c);
						if(SystemExec(c)) {
							m_statestr = cString::sprintf(tr("Failed to save configuration for Netceiver %s"), uuid);
							m_state = 0;
							continue;
						} // if
						m_state=1;
						break;
					} // case 4
				} // switch
			} // while
			m_state = 0;
		}; // Action
		virtual bool PatchAllCamFlags(xmlDocPtr doc, const char *uuid) {
			for(cOsdItem *i=m_menu->First(); i; i=m_menu->Next(i)) {
				cCamMtd *m = dynamic_cast<cCamMtd *>(i);
				if(m && !strcmp(m->m_uuid, uuid)) {
					if(!PatchCamFlags(doc, uuid, itoa(m->m_slot), itoa(m->m_flags)))
						return false;
					m->MtdSaved();
				} // if
			} // for
			return true;
		}; // PatchAllCamFlags
		virtual bool PatchCamFlags(xmlDocPtr doc, const char *uuid, const char *slot, const char *flags) {
			bool uuid_match=false;
			bool flags_set =false;
			xmlNode *node = xmlDocGetRootElement(doc);
			node = node ? node->children : NULL;
			while(node && xmlStrcmp(node->name, (const xmlChar *)"Description"))
				node=node->next;
			node = node ? node->children : NULL;
			while(node) {
				if(node && !xmlStrcmp(node->name, (const xmlChar *)"component")) {
					xmlNode *item = node->children;
					while(item && xmlStrcmp(item->name, (const xmlChar *)"Description")) {
						item = item->next;
					} // while
					xmlChar *about = item ? xmlGetProp(item, (const xmlChar *)"about") : NULL;
					if(about) {
						if (!xmlStrcmp(about, (const xmlChar *)"Platform")) {
							xmlNode *sub = item->children;
							while(sub) {
								if(!xmlStrcmp(sub->name, (const xmlChar *)"UUID")) {
									xmlChar *value=xmlNodeListGetString(doc, sub->children, 1);
									if(value) {
										uuid_match=!xmlStrcmp(value, (const xmlChar *)uuid); 
										xmlFree(value);
									} // if
								} // if
								sub = sub->next;
							} // while
						} else if(!xmlStrcmp(about, (const xmlChar *)"CAM")) {
							xmlNode *sub = item->children;
							while(sub) {
								if(!xmlStrcmp(sub->name, (const xmlChar *)"Slot")) {
									xmlChar *value=xmlNodeListGetString(doc, sub->children, 1);
									if(value) {
										if (!xmlStrcmp(value, (const xmlChar *)slot)) {
											xmlNode *tst = item->children;
											while(tst) {
												if(!xmlStrcmp(tst->name, (const xmlChar *)"Flags")) {
													xmlReplaceNode(tst, xmlNewChild(item, xmlSearchNs(doc, tst, (const xmlChar *)"prf"), (const xmlChar *)"Flags", (const xmlChar *)flags));
													xmlFreeNode(tst);
													flags_set=true;
													tst = NULL;
													continue;
												} // if
												tst = tst->next;
											} // while
										} // if
										xmlFree(value);
									} // if
								} // if
								sub = sub->next;
							} // while
						} // if
						xmlFree(about);
					} // if
				} // if
				node = node->next;
			} // while
			return uuid_match && flags_set;
		}; // PatchCamFlags
		cCamMenu *m_menu;
		const char *m_iface;
		unsigned int m_state;
		cString m_statestr;
}; // cNCUpdate

cCamMenu::cCamMenu (cmdline_t * cmd) 
#if APIVERSNUM < 10700
: cOsdMenu (trVDR ("Common Interface"), 18)
#else
: cOsdMenu (trVDR ("Common Interface"), 23)
#endif
{
	NCUpdate=NULL;
	m_cmd = cmd;
	inCamMenu = false;
	inMMIBroadcastMenu = false;
	inputRequested = eInputNone;
	end = false;
	currentSelected = -1;
	pinCounter = 0;
	alreadyReceived = false;
	mmi_session = -1;
	SetNeedsFastResponse (true);
	CamFind ();
}

cCamMenu::cCamMenu (cmdline_t * cmd, mmi_info_t * mmi_info):cOsdMenu (trVDR ("Common Interface"), 18)
{
	NCUpdate=NULL;
	m_cmd = cmd;
	inCamMenu = false;
	inMMIBroadcastMenu = false;
	inputRequested = eInputNone;
	end = false;
	currentSelected = -1;
	pinCounter = 0;
	alreadyReceived = false;
	mmi_session = -1;
	SetNeedsFastResponse (true);
	mmi_session = CamMenuOpen (mmi_info);
}


cCamMenu::~cCamMenu ()
{
	CamMenuClose (mmi_session);
}

void cCamMenu::OpenCamMenu ()
{
	bool timeout = true;
	cCamInfo *item = dynamic_cast<cCamInfo *>(Get(currentSelected));
	mmi_session = item ? item->CamMenuOpen (m_cmd->iface) : 0;
	char buf2[MMI_TEXT_LENGTH * 2];
	printf ("mmi_session: %d\n", mmi_session);
	if (mmi_session > 0) {
		Clear ();
		Skins.Message(mtWarning, trVDR("Opening CAM menu..."));
		inCamMenu = true;
		time_t t = time (NULL);
		while ((time (NULL) - t) < CAMMENU_TIMEOUT) {
			memset(buf2, 0, sizeof(buf2));
			// receive the CAM MENU
			if (CamMenuReceive (mmi_session, buf, MMI_TEXT_LENGTH) > 0) {
				cCharSetConv conv = cCharSetConv ("ISO-8859-1", "UTF-8");
				conv.Convert (buf, buf2, MMI_TEXT_LENGTH * 2);
				char *saveptr = NULL;
				char *ret = strtok_r (buf2, "\n", &saveptr);
				if (ret) {
					Add (new cOsdItem (ret));
					while (ret != NULL) {
						ret = strtok_r (NULL, "\n", &saveptr);
						if (ret)
							Add (new cOsdItem (ret));
					}
				}
				timeout = false;
				break;
			}
		}
	}
	if (mmi_session && timeout) {
		printf ("%s: Error\n", __PRETTY_FUNCTION__);
		Clear ();
		Add (new cOsdItem (trVDR ("Error")));
	}
	Display ();
}

void cCamMenu::Receive ()
{
	bool timeout = true;
	if (mmi_session > 0) {
		char buf2[MMI_TEXT_LENGTH * 2];
		time_t t = time (NULL);
		while ((time (NULL) - t) < CAMMENU_TIMEOUT) {
			memset(buf2, 0, sizeof(buf2));
			// receive the CAM MENU
			if (alreadyReceived || CamMenuReceive (mmi_session, buf, MMI_TEXT_LENGTH) > 0) {
				Clear ();
				alreadyReceived = false;
				printf ("MMI: \"%s\"\n", buf);
				if (!strncmp (buf, "blind = 0", 9))
					inputRequested = eInputNotBlind;
				else if (!strncmp (buf, "blind = 1", 9))
					inputRequested = eInputBlind;
				cCharSetConv conv = cCharSetConv ("ISO-8859-1", "UTF-8");
				conv.Convert (inputRequested ? buf + 28 : buf, buf2, MMI_TEXT_LENGTH * 2);
				printf ("MMI-UTF8: \"%s\"\n", buf2);
				if (!strcmp (buf, "end")) {
					  /** The Alphacrypt returns "end" when pressing "Back" in it's main menu */
					end = true;
					return;
				}
				char *saveptr = NULL;
				char *ret = strtok_r (buf2, "\n", &saveptr);
				if (ret) {
					Add (new cOsdItem (ret));
					while (ret != NULL) {
						ret = strtok_r (NULL, "\n", &saveptr);
						if (ret)
							Add (new cOsdItem (ret));
					}
				}
				timeout = false;
				break;
			}
		}
	}
	if (timeout) {
		printf ("%s: mmi_session: %i\n", __PRETTY_FUNCTION__, mmi_session);
		Add (new cOsdItem (trVDR ("Error")));
	}
	Display ();
}

int cCamMenu::CamFind ()
{
	Clear ();
	int n, cnt = 0, i;
	netceiver_info_list_t *nc_list = nc_get_list ();
	//printf ("Looking for netceivers out there....\n");
	nc_lock_list ();
	for (n = 0; n < nc_list->nci_num; n++) {
		netceiver_info_t *nci = nc_list->nci + n;
		//printf ("\nFound NetCeiver: %s \n", nci->uuid);
		char buf[512]; // avoid -Wformat-truncation
		Add (new cOsdItem ("Netceiver", osUnknown, false));
		snprintf (buf, sizeof(buf), "    %s: %s", "ID", nci->uuid); // avoid -Wformat-truncation
		Add (new cOsdItem (buf, osUnknown, false));
		Add (new cOsdItem ("", osUnknown, false));
		printf ("    CAMS [%d]: \n", nci->cam_num);
		int nrAlphaCrypts = 0;
		int nrOtherCams = 0;

		for (i = nci->cam_num - 1; i >= 0 /*nci->cam_num */ ; i--) {
			if((2==nci->cam[i].status)&&strlen(nci->cam[i].menu_string)) {
				if (cCamInfo::MtdPossible(nci->cam[i].menu_string))
					nrAlphaCrypts++;
				else
					nrOtherCams++;
			} // if
		} // for
		bool mtdImpossible = nrAlphaCrypts > 0 && nrOtherCams > 0; // as in netcvmenu.c
		for (i = nci->cam_num - 1; i >= 0 /*nci->cam_num */ ; i--) {
			switch (nci->cam[i].status) {
				case 2:	{//DVBCA_CAMSTATE_READY:
					cCamInfo *item = new cCamInfo(nci->uuid, nci->cam[i].slot, nci->cam[i].menu_string);
					Add(item);
					if(!mtdImpossible && item->MtdPossible())
						Add(new cCamMtd(nci->uuid, nci->cam[i].slot, nci->cam[i].menu_string, nci->cam[i].flags));
					break;
				}
				case 0: // no cam?
					Add(new cCamInfo(nci->uuid, nci->cam[i].slot, NULL));
					break;
				default:
					Add(new cCamInfo(nci->uuid, nci->cam[i].slot, nci->cam[i].menu_string));
			}
			Add (new cOsdItem ("", osUnknown, false));
			cnt++;
		}
		if (mtdImpossible) {
			Add(new cOsdItem(cString::sprintf("   %s", tr("Multi-Transponder-Decryption is"))));
			Add(new cOsdItem(cString::sprintf("   %s", tr("impossible because of mixed CAMs"))));
			Add (new cOsdItem ("", osUnknown, false));
		} // if
	}
	nc_unlock_list ();
	Display ();
	return cnt;
}

int cCamMenu::CamMenuOpen (mmi_info_t * mmi_info)
{
	//printf ("Opening CAM Menu at NetCeiver %s Slot %d\n", mmi_info->uuid, mmi_info->slot);

	char buf[MMI_TEXT_LENGTH * 2];

	inMMIBroadcastMenu = true;
	inCamMenu = true;
	cCharSetConv conv = cCharSetConv ("ISO-8859-1", "UTF-8");
	conv.Convert (mmi_info->mmi_text, buf, MMI_TEXT_LENGTH * 2);
	//printf ("MMI-UTF8: \"%s\"\n", buf);
	char *saveptr = NULL;
	char *ret = strtok_r (buf, "\n", &saveptr);
	if (ret) {
		Add (new cOsdItem (ret));
		while (ret != NULL) {
			ret = strtok_r (NULL, "\n", &saveptr);
			if (ret)
				Add (new cOsdItem (ret));
		}
	}

	int mmi_session = mmi_open_menu_session (mmi_info->uuid, m_cmd->iface, 0, mmi_info->slot);

	//printf ("CamMenuOpen: mmi_session: %i\n", mmi_session);
	return mmi_session;
}

int cCamMenu::CamMenuSend (int mmi_session, const char *c)
{
	return mmi_send_menu_answer (mmi_session, (char *) c, strlen (c));
}

int cCamMenu::CamMenuReceive (int mmi_session, char *buf, int bufsize)
{
	return mmi_get_menu_text (mmi_session, buf, bufsize, 50000);
}

void cCamMenu::CamMenuClose (int mmi_session)
{
	close (mmi_session);
}

int cCamMenu::CamPollText (mmi_info_t * text)
{
	return mmi_poll_for_menu_text (m_cam_mmi, text, 10);
}

eOSState cCamMenu::ProcessKey (eKeys Key)
{
	if(NCUpdate) {
		if(NCUpdate->Done()) {
			SetStatus("");
			Skins.Message(mtWarning, NCUpdate->GetStateStr());
			delete NCUpdate;
			NCUpdate=NULL;
			return osBack;
		} else {
			SetStatus(NCUpdate->GetStateStr());
			Display();
			return osContinue;
		};
	} // if
	eOSState ret = cOsdMenu::ProcessKey (Key);

	currentSelected = Current ();

	if (end) {
		end = false;
		inCamMenu = false;
		if (inMMIBroadcastMenu)
			return osEnd;
		CamMenuClose (mmi_session);
		CamFind ();
		return osContinue;
	}
	bool MtdModified=false;
	for(cOsdItem *i=First(); i; i=Next(i)){
		cCamMtd *m = dynamic_cast<cCamMtd *>(i);
		if(m&&m->MtdModified()) MtdModified=true;
	} // for
	cCamInfo *info = dynamic_cast<cCamInfo *>(Get(Current()));
	SetHelp(MtdModified?tr("Save"):NULL, NULL, (info&&info->CanCamReset()) ? trVDR("Reset") : NULL, NULL);
	switch (Key) {
#if 0
    case kUp:
    case kDown:
        {
      	unsigned int nrInCamList = currentSelected - ((int) currentSelected / 5) * 3 - 3;	// minus the empty rows
        if(strlen(cam_list[nrInCamList].info))
            SetHelp(trVDR("Reset"), NULL, NULL, NULL);
        else
            SetHelp(NULL, NULL, NULL, NULL);
        break;
        }
#endif
	case kRed: { // modify mtd settings
		NCUpdate = new cNCUpdate(this, m_cmd->iface);
		SetHelp(NULL, NULL, NULL, NULL);
		SetStatus(tr("Updating configuration..."));
		Display();
		return osContinue;
	}
	case kYellow: {
		cCamInfo *item = dynamic_cast<cCamInfo *>(Get(currentSelected));
		if(item) item->CamReset(m_cmd->iface);
		break;
	}
	case kOk:
		SetStatus ("");
		pinCounter = 0;	// reset pin
		if (inCamMenu && inputRequested) {
			inputRequested = eInputNone;	// input was sent
			//printf ("Sending pin: \"%s\"\n", pin);
			CamMenuSend (mmi_session, pin);
			Receive ();
		} else if (inMMIBroadcastMenu) {
			//printf ("Sending: \"%s\"\n", Get (Current())->Text());
			CamMenuSend (mmi_session, Get (Current ())->Text ());
			Receive ();
		} else if (inCamMenu) {
			//printf ("Sending: \"%s\"\n", Get (Current ())->Text ());
				if (strcmp(Get ( Current ())->Text(), trVDR("Error"))) // never send Error...
				CamMenuSend (mmi_session, Get (Current ())->Text ());
			Receive ();
		} else
			OpenCamMenu ();
		break;
	case kBack:
		pinCounter = 0;	// reset pin
		if (!inCamMenu)
			return osBack;
		inCamMenu = false;
		SetStatus ("");
		CamMenuClose (mmi_session);
		CamFind ();
		return osContinue;
	case k0...k9:
		if (inputRequested) {
			pin[pinCounter++] = 48 + Key - k0;
			pin[pinCounter] = '\0';
			printf ("key: %c, pin: \"%s\"\n", 48 + Key - k0, pin);
			char buf[16];
			int i;
			for (i = 0; i < pinCounter; i++)
				(inputRequested == eInputBlind) ? buf[i] = '*' : buf[i] = pin[i];
			buf[i] = '\0';
			SetStatus (buf);
		} else {
			pinCounter = 0;	// reset pin
			SetStatus ("");
			for (int i = 0; Get (i); i++) {
				const char *txt = Get (i)->Text ();
				if (txt[0] == 48 + Key - k0 && txt[1] == '.') {
					CamMenuSend (mmi_session, txt);
					Receive ();
				}
			}
		}
		break;
	default:
		int bla = 0;
		if (mmi_session > 0) {
			bla = CamMenuReceive (mmi_session, buf, MMI_TEXT_LENGTH);
			if (bla > 0) {
				alreadyReceived = true;
				//printf ("bla: %i\n", bla);
				Receive ();
			}
		}
		break;
	}
	return ret;
}
