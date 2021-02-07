/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

/*
 * mcli.c: A plugin for the Video Disk Recorder
 */

#include <vdr/plugin.h>
#include <vdr/player.h>

#include "filter.h"
#include "device.h"
#include "cam_menu.h"
#include "mcli_service.h"
#include "mcli.h"
#include <sstream>


static int reconf = 0;
int m_debugmask = 0;
int m_logskipmask = 0;
bool m_cam_disable;

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------

class cMenuSetupMcli:public cMenuSetupPage
{
      private:
	cmdline_t * m_cmd;
      protected:
	virtual void Store (void);
      public:
	cMenuSetupMcli (cmdline_t * cmd);
};

cMenuSetupMcli::cMenuSetupMcli (cmdline_t * cmd)
{
	m_cmd = cmd;
	Add (new cMenuEditIntItem (trNOOP ("DVB-C"), &m_cmd->tuner_type_limit[FE_QAM]));
	Add (new cMenuEditIntItem (trNOOP ("DVB-T"), &m_cmd->tuner_type_limit[FE_OFDM]));
	Add (new cMenuEditIntItem (trNOOP ("DVB-S"), &m_cmd->tuner_type_limit[FE_QPSK]));
	Add (new cMenuEditIntItem (trNOOP ("DVB-S2"), &m_cmd->tuner_type_limit[FE_DVBS2]));
}

void cMenuSetupMcli::Store (void)
{
	SetupStore ("DVB-C", m_cmd->tuner_type_limit[FE_QAM]);
	SetupStore ("DVB-T", m_cmd->tuner_type_limit[FE_OFDM]);
	SetupStore ("DVB-S", m_cmd->tuner_type_limit[FE_QPSK]);
	SetupStore ("DVB-S2", m_cmd->tuner_type_limit[FE_DVBS2]);
	reconf = 1;
}

cOsdObject *cPluginMcli::AltMenuAction (void)
{
	// Call this code periodically to find out if any CAM out there want's us to tell something.
	// If it's relevant to us we need to check if any of our DVB-Devices gets programm from a NetCeiver with this UUID.
	// The text received should pop up via OSD with a CAM-Session opened afterwards (CamMenuOpen...CamMenuReceive...CamMenuSend...CamMenuClose).
	mmi_info_t m;
	if (CamPollText (&m) > 0) {
		isyslog ("NetCeiver %s CAM slot %d Received %s valid for:\n", m.uuid, m.slot, m.mmi_text);
		for (int i = 0; i < m.caid_num; i++) {
			caid_mcg_t *c = m.caids + i;
			int satpos;
			fe_type_t type;
			recv_sec_t sec;
			struct dvb_frontend_parameters fep;
			int vpid;

			mcg_get_satpos (&c->mcg, &satpos);
			mcg_to_fe_parms (&c->mcg, &type, &sec, &fep, &vpid);

			for (cMcliDeviceObject * dev = m_devs.First (); dev; dev = m_devs.Next (dev)) {
				cMcliDevice *d = dev->d ();
#ifdef DEBUG_TUNE
				DEBUG_MASK(DEBUG_BIT_TUNE,
				dsyslog("mcli::%s: satpos: %i vpid: %i fep.freq: %i", __FUNCTION__, satpos, vpid, fep.frequency);
				)
#endif
				struct in6_addr mcg = d->GetTenData ()->mcg;
				mcg_set_id (&mcg, 0);

#ifdef DEBUG_TUNE
				DEBUG_MASK(DEBUG_BIT_TUNE,
				char str[INET6_ADDRSTRLEN];
				inet_ntop (AF_INET6, &c->mcg, str, INET6_ADDRSTRLEN);
				dsyslog ("mcli::%s: MCG from MMI: %s", __FUNCTION__, str);
				inet_ntop (AF_INET6, &mcg, str, INET6_ADDRSTRLEN);
				dsyslog ("mcli::%s: MCG from DEV: %s", __FUNCTION__, str);
				)
#endif

				if (IN6_IS_ADDR_UNSPECIFIED (&c->mcg) || !memcmp (&c->mcg, &mcg, sizeof (struct in6_addr)))
					return new cCamMenu (&m_cmd, &m);
			}
#ifdef DEBUG_TUNE
			DEBUG_MASK(DEBUG_BIT_TUNE,
			dsyslog ("mcli::%s: SID/Program Number:%04x, SatPos:%d Freqency:%d", __FUNCTION__, c->caid, satpos, fep.frequency);
			)
#endif
		}
		if (m.caid_num && m.caids) {
			free (m.caids);
		}
	}
	return NULL;
}

int cPluginMcli::CamPollText (mmi_info_t * text)
{
	if (m_mmi_init_done && !reconf) {
		return mmi_poll_for_menu_text (m_cam_mmi, text, 10);
	} else {
		return 0;
	}
}

cPluginMcli::cPluginMcli (void)
{
	int i;
	//init parameters
	memset (&m_cmd, 0, sizeof (cmdline_t));

	for (i = 0; i <= FE_DVBS2; i++) {
		m_cmd.tuner_type_limit[i] = MCLI_MAX_DEVICES;
	}
	m_cmd.port = 23000;
	m_cmd.mld_start = 1;
	m_mmi_init_done = 0;
	m_recv_init_done = 0;
	m_mld_init_done = 0;
	m_api_init_done = 0;
	m_tuner_max = MCLI_MAX_DEVICES;
        m_cam_present = false;
	memset (m_cam_pool, 0, sizeof (cam_pool_t) * CAM_POOL_MAX);
	for(i=0; i<CAM_POOL_MAX; i++) {
		m_cam_pool[i].max = -1;
	}
	strcpy (m_cmd.cmd_sock_path, API_SOCK_NAMESPACE);
	memset (m_tuner_pool, 0, sizeof(tuner_pool_t)*TUNER_POOL_MAX);
	for(i=0; i<TUNER_POOL_MAX; i++) {
		m_tuner_pool[i].type = -1;
	}
}

cPluginMcli::~cPluginMcli ()
{
	ExitMcli ();

}

bool cPluginMcli::InitMcli (void)
{
	if(m_recv_init_done && (m_mld_init_done || !m_cmd.mld_start) && m_api_init_done && m_mmi_init_done) return true;
	int ifacelen = strlen(m_cmd.iface);
	if(ifacelen) { // Check if iface exists
		FILE *file = fopen("/proc/net/if_inet6", "r");
		if(!file) return false;
		bool found = false;
		char buf[255];
		while(fgets(buf, sizeof(buf), file)) {
			int buflen = strlen(buf);
			while(buf[buflen-1]=='\n') buf[--buflen] = 0;
			while(buf[buflen-1]=='\r') buf[--buflen] = 0;
			while(buf[buflen-1]==' ' ) buf[--buflen] = 0;
			if((buflen >= ifacelen) && (!strcmp(&buf[buflen-ifacelen], m_cmd.iface))) {
				found = true;
				break;
			}
		}
		fclose(file);
		if(!found) return false;
	}
	// Ok, iface exists so go on
	if (!m_recv_init_done) {
		if(!recv_init (m_cmd.iface, m_cmd.port))
			m_recv_init_done = 1;
		else return false;
	}
	if (!m_mld_init_done && m_cmd.mld_start) {
		if(!mld_client_init (m_cmd.iface))
			m_mld_init_done = 1;
		else return false;
	}
	if (!m_api_init_done) {
		if(!api_sock_init (m_cmd.cmd_sock_path))
			m_api_init_done = 1;
		else return false;
	}
	if(!m_mmi_init_done) {
		if((m_cam_mmi = mmi_broadcast_client_init (m_cmd.port, m_cmd.iface)) != NULL)
			m_mmi_init_done = 1;
		else return false;
	}
	for(int i=m_devs.Count(); i < m_tuner_max; i++) {
		cMcliDevice *m = NULL;
		cPluginManager::CallAllServices ("OnNewMcliDevice-" MCLI_DEVICE_VERSION, &m);
		if(!m) {
			m = new cMcliDevice;
		}
		if(m) {
			m->SetMcliRef (this);
			cMcliDeviceObject *d = new cMcliDeviceObject (m);
			m_devs.Add (d);
		}
	}
	return true;
}

void cPluginMcli::ExitMcli (void)
{
	if (m_mmi_init_done) {
		mmi_broadcast_client_exit (m_cam_mmi);
		m_mmi_init_done = 0;
	}
	if (m_api_init_done) {
		api_sock_exit ();
		m_api_init_done = 0;
	}
	if (m_mld_init_done) {
		mld_client_exit ();
		m_mld_init_done = 0;
	}
	if (m_recv_init_done) {
		recv_exit ();
		m_recv_init_done = 0;
	}
}

const char *cPluginMcli::CommandLineHelp (void)
{
	return (
		"  --ifname <network interface>\n"
		"  --port <port> (default: -port 23000)\n"
		"  --dvb-s <num> --dvb-c <num> --dvb-t <num> --atsc <num> --dvb-s2 <num> (limit number of device types - default: 8 of every type)\n"
		"  --tuner-max <num> (limit number of tuners, default: 8)\n"
		"  --mld-reporter-disable\n"
		"  --sock-path <filepath>\n"
		"  --debugmask <value> (decimal or hex debug mask)\n"
		"  --logskipmask <value> (decimal or hex log skip mask)\n"
		"\n"
	);
}

bool cPluginMcli::ProcessArgs (int argc, char *argv[])
{
	int tuners = 0, i;
	char c;
	int ret;
	while (1) {
		//int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"port", 1, 0, 0},	//0
			{"ifname", 1, 0, 0},	//1
			{"dvb-s", 1, 0, 0},	//2
			{"dvb-c", 1, 0, 0},	//3
			{"dvb-t", 1, 0, 0},	//4
			{"atsc", 1, 0, 0},	//5
			{"dvb-s2", 1, 0, 0},	//6
			{"mld-reporter-disable", 0, 0, 0},	//7
			{"sock-path", 1, 0, 0},	//8
			{"tuner-max", 1, 0, 0},	//9
			{"debugmask", 1, 0, 0}, //10: debug mask for selective debugging, see mcli.h
			{"cam-disable", 0, 0, 0}, //11: disable use of CAM (skip channels)
			{"logskipmask", 1, 0, 0},   //12: log mask for selective disabling of logging, see mcli.h
			{NULL, 0, 0, 0}
		};

		ret = getopt_long_only (argc, argv, "", long_options, &option_index);
		c = (char) ret;
		if (ret == -1) {
			break;
		}
		if (c == '?') {
			continue;
		}

		switch (option_index) {
		case 0:
			m_cmd.port = atoi (optarg);
			break;
		case 1:
			strncpy (m_cmd.iface, optarg, IFNAMSIZ - 1);
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			i = atoi (optarg);
			if (!tuners) {
				memset (m_cmd.tuner_type_limit, 0, sizeof (m_cmd.tuner_type_limit));
			}
			m_cmd.tuner_type_limit[option_index - 2] = i;
			tuners += i;
			break;
		case 7:
			m_cmd.mld_start = 0;
			break;
		case 8:
			strncpy (m_cmd.cmd_sock_path, optarg, _POSIX_PATH_MAX - 1);
			break;
		case 9:
			m_tuner_max = atoi (optarg);
			break;
		case 10:
			if ((strlen(optarg) > 2) && (strncasecmp(optarg, "0x", 2) == 0)) {
				// hex conversion
				if (sscanf(optarg + 2, "%x", &m_debugmask) == 0) {
					isyslog("mcli::%s: can't parse hexadecimal debug mask (skip): %s", __FUNCTION__, optarg);
				};
			} else {
				m_debugmask = atoi (optarg);
			};
			dsyslog("mcli::%s: enable debug mask: %d (0x%02x)", __FUNCTION__, m_debugmask, m_debugmask);
			break;
		case 11:
			m_cam_disable = true;
			dsyslog("mcli::%s: enable 'm_cam_disable'", __FUNCTION__);
			break;
		case 12:
			if ((strlen(optarg) > 2) && (strncasecmp(optarg, "0x", 2) == 0)) {
				// hex conversion
				if (sscanf(optarg + 2, "%x", &m_logskipmask) == 0) {
					isyslog("mcli::%s: can't parse hexadecimal log mask (skip): %s", __FUNCTION__, optarg);
				};
			} else {
				m_logskipmask = atoi (optarg);
			};
			isyslog("mcli::%s: enable log skip mask: %d (0x%02x)", __FUNCTION__, m_logskipmask, m_logskipmask);
			break;
		default:
			dsyslog ("MCli::%s: ?? getopt returned character code 0%o ??\n", __FUNCTION__, c);
		}
	}
	// Implement command line argument processing here if applicable.
	return true;
}

cam_pool_t *cPluginMcli::CAMFindByUUID (const char *uuid, int slot)
{
	cam_pool_t *cp;
	for(int i=0; i<CAM_POOL_MAX; i++) {
		cp = m_cam_pool + i;
		if(cp->max >= 0 && !strcmp(cp->uuid, uuid) && (slot == -1 || slot == cp->slot)) {
			return cp;
		}
	}
	return NULL;
}

cam_pool_t *cPluginMcli::CAMPoolFindFree(void)
{
	for(int i=0; i<CAM_POOL_MAX; i++) {
		cam_pool_t *cp = m_cam_pool + i;
		if(cp->max == -1) {
			return cp;
		}
	}
	return NULL;
}
int cPluginMcli::CAMPoolAdd(netceiver_info_t *nci)
{
	bool update = false;
	int ret = 0;
	for(int j=0; j < nci->cam_num; j++) {
		update = false;
		cam_pool_t *cp=CAMFindByUUID(nci->uuid, nci->cam[j].slot);
		if(!cp) {
			cp=CAMPoolFindFree();
			if(!ret) {
				ret = 1;
			}
		} else {
			update = true;
			ret = 2;
		}
		if(!cp){
			return ret;
		}
		if (nci->cam[j].status) {
			switch (nci->cam[j].flags) {
				case CA_SINGLE:
				case CA_MULTI_SID:
					m_cam_present = true;
#ifdef DEBUG_RESOURCES
					DEBUG_MASK(DEBUG_BIT_RESOURCES,
					if (!update) dsyslog("mcli::%s: Found CAM %d", __FUNCTION__, j);
					)
#endif
					cp->max = 1;
					break;
				case CA_MULTI_TRANSPONDER:
#ifdef DEBUG_RESOURCES
					DEBUG_MASK(DEBUG_BIT_RESOURCES,
					if (!update) dsyslog("mcli::%s: Found CAM %d", __FUNCTION__, j);
					)
#endif
					m_cam_present = true;
					cp->max = nci->cam[j].max_sids;
					break;
			}
		} else {
			cp->max = 0;
		}
		cp->status = nci->cam[j].status;
		if(!update) {
			cp->slot = nci->cam[j].slot;
			strcpy(cp->uuid, nci->uuid);
			cp->use = 0;
		}
	}
	return ret;
}
bool cPluginMcli::CAMPoolDel(const char *uuid)
{
	cam_pool_t *cp;
	bool ret=false;
	for(int i=0; i<CAM_POOL_MAX; i++) {
		cp = m_cam_pool + i;
		if(cp->max>=0 && !strcmp(cp->uuid, uuid)) {
			cp->max = -1;
			ret=true;
		}
	}
	return ret;
}

cam_pool_t *cPluginMcli::CAMAvailable (const char *uuid, int slot, bool lock)
{
	cam_pool_t *ret = NULL;
	if(lock) {
		Lock();
	}
	cam_pool_t *cp;
	for(int i=0; i<CAM_POOL_MAX; i++) {
		cp = m_cam_pool + i;
		if(cp->max>0 && (!uuid || !strcmp(cp->uuid, uuid)) && (slot == -1 || (cp->slot == slot))) {
			if((cp->max - cp->use) > 0){
				ret = cp;
				break;
			}
		}
	}
#ifdef DEBUG_RESOURCES
	DEBUG_MASK(DEBUG_BIT_RESOURCES,
	if(ret) {
		dsyslog("mcli::%s: Available CAM [%s]:%d -> [%s]:%d", __FUNCTION__, uuid, slot, ret->uuid, ret->slot);
	}
	)
#endif
	if(lock) {
		Unlock();
	}
	return ret;
}

cam_pool_t *cPluginMcli::CAMAlloc (const char *uuid, int slot)
{
	LOCK_THREAD;
	cam_pool_t *cp = NULL;
	if ((cp = CAMAvailable (uuid, slot, false))) {
		cp->use++;
	}

#ifdef DEBUG_RESOURCES
	DEBUG_MASK(DEBUG_BIT_RESOURCES,
        if(cp) {
		dsyslog ("mcli::%s: AllocateCAM [%s]:%d -> [%s]:%d", __FUNCTION__, uuid, slot, cp->uuid, cp->slot);
        } else {
		dsyslog ("mcli::%s: AllocateCAM [%s]:%d -> FAIL", __FUNCTION__, uuid, slot);
	}
	)
#endif

	return cp;
}

int cPluginMcli::CAMFree (cam_pool_t *cp)
{
	LOCK_THREAD;
#ifdef DEBUG_RESOURCES
	DEBUG_MASK(DEBUG_BIT_RESOURCES,
	dsyslog ("mcli::%s: FreeCAM [%s]:%d", __FUNCTION__, cp->uuid, cp->slot);
	)
#endif
	if (cp->use > 0) {
		cp->use--;
	}
	return cp->use;
}

bool cPluginMcli::CAMSteal(const char *uuid, int slot, bool force)
{
		for (cMcliDeviceObject * d = m_devs.First (); d; d = m_devs.Next (d)) {
			cam_pool_t *cp=d->d()->GetCAMref();
			if(d->d()->Priority()<0 && d->d()->GetCaEnable() && (slot == -1 || slot == cp->slot)) {
#ifdef DEBUG_RESOURCES
				DEBUG_MASK(DEBUG_BIT_RESOURCES,
				dsyslog("mcli::%s: Can Steal CAM on slot %d from DVB %d", __FUNCTION__, slot, d->d()->CardIndex()+1);
				)
#endif
				if(force) {
					d->d ()->SetTempDisable (true);
#ifdef DEBUG_RESOURCES
					DEBUG_MASK(DEBUG_BIT_RESOURCES,
					dsyslog("mcli::%s: Stole CAM on slot %d from DVB %d", __FUNCTION__, cp->slot, d->d()->CardIndex()+1);
					)
#endif
				}
				return true;
			}
		}
		return false;
}

satellite_list_t *cPluginMcli::TunerFindSatList(const netceiver_info_t *nc_info, const char *SatelliteListName) const
{
	if(SatelliteListName == NULL) {
		return NULL;
	}

	//for (int i = 0; i < nc_info->sat_list_num; i++) {
	//	printf("mcli::%s: Satlist Name %s\n", __FUNCTION__, nc_info->sat_list[i].Name);
	//}	

	for (int i = 0; i < nc_info->sat_list_num; i++) {
		if (!strcmp (SatelliteListName, nc_info->sat_list[i].Name)) {
			//printf ("found uuid in sat list %d\n", i);
			return nc_info->sat_list + i;
		}
	}
	return NULL;
}

bool cPluginMcli::SatelitePositionLookup(const satellite_list_t *satlist, int pos) const
{
	if(satlist == NULL) {
		return false;
	}

	for(int i=0; i<satlist->sat_num;i ++) {
		satellite_info_t *s=satlist->sat+i;

		//printf("mcli::%s: Satlist Pos %d s->type %d s->Name %s s->SatPos %d LNB%d UNI%d ROT%d\n", __FUNCTION__, pos, s->type, s->Name,  s->SatPos, SAT_SRC_LNB, SAT_SRC_UNI, SAT_SRC_ROTOR);

		switch(s->type){
			case SAT_SRC_LNB:
			case SAT_SRC_UNI:
				if(pos == s->SatPos) {
					//printf("satlist found\n");
					return true;
				}
				break;
			case SAT_SRC_ROTOR:
				if(pos>=s->SatPosMin && pos <=s->SatPosMax) {
					//printf("satlist found\n");
					return true;
				}
				break;
		}
	}
	//printf("satlist not found\n");

	return false;
}

bool cPluginMcli::TunerSatelitePositionLookup(tuner_pool_t *tp, int pos) const
{
	if((tp->type != FE_QPSK) && (tp->type != FE_DVBS2)) {
		return true; // if not DVB-S or DVB-S2 return true
	}
	if(pos == NO_SAT_POS) {
		return true;
	}

	nc_lock_list ();
	netceiver_info_list_t *nc_list = nc_get_list ();
	satellite_list_t *satlist=NULL;
	for (int n = 0; n < nc_list->nci_num; n++) {
		netceiver_info_t *nci = nc_list->nci + n;
		int l=strlen(tp->uuid)-5;
		if(strncmp(nci->uuid, tp->uuid, l)) {
			continue;
		}
		satlist=TunerFindSatList(nci, tp->SatListName);
		if(satlist) {
			break;
		}
	}
	bool ret;
	if(satlist == NULL) {
                esyslog("mcli::%s: No Satlist found \n", __FUNCTION__);
		ret = false;
	} else {
		ret=SatelitePositionLookup(satlist, pos);
		if (!ret) {
			esyslog("mcli::%s: Pos %d not found in Satlist \n", __FUNCTION__, pos);
		}		
	}
	nc_unlock_list ();
	return ret;
}
tuner_pool_t *cPluginMcli::TunerFindByUUID (const char *uuid)
{
	tuner_pool_t *tp;
	for(int i=0; i<TUNER_POOL_MAX; i++) {
		tp=m_tuner_pool+i;
		if(tp->type != -1 && !strcmp(tp->uuid, uuid)) {
			return tp;
		}
	}
	return NULL;
}

bool cPluginMcli::Ready()
{
	for(int i=0; i<CAM_POOL_MAX; i++)
		if((m_cam_pool[i].max >= 0) && (DVBCA_CAMSTATE_INITIALISING == m_cam_pool[i].status)) return false;
	for (cMcliDeviceObject * d = m_devs.First (); d; d = m_devs.Next (d))
		if(d->d ()->HasInput()) return true;
	return false;
}

#define MAX_TUNER_TYPE_COUNT (FE_DVBS2+1)
int cPluginMcli::TunerCount() {
	tuner_pool_t *tp;
	int tct[MAX_TUNER_TYPE_COUNT];
	memset(&tct, 0, sizeof(tct));
	for(int i=0; i<TUNER_POOL_MAX; i++) {
		tp=m_tuner_pool+i;
		if((tp->type >= 0) && (tp->type < MAX_TUNER_TYPE_COUNT))
			if(tct[tp->type] < m_cmd.tuner_type_limit[tp->type])
				tct[tp->type]++;
	}
	int tc=0;
	for(int i=0; i<MAX_TUNER_TYPE_COUNT; i++)
		tc+=tct[i];
	return tc;
}

int cPluginMcli::TunerCountByType (const fe_type_t type)
{
	int ret=0;
	tuner_pool_t *tp;
	for(int i=0; i<TUNER_POOL_MAX; i++) {
		tp=m_tuner_pool+i;
		if(tp->inuse && tp->type == type) {
			ret++;
		}
	}
	return ret;
}

bool  cPluginMcli::TunerPoolAdd(tuner_info_t *t)
{
	tuner_pool_t *tp;
	for(int i=0; i<TUNER_POOL_MAX; i++) {
		tp=m_tuner_pool+i;
		if(tp->type == -1) {
			tp->type=t->fe_info.type;
			strcpy(tp->uuid, t->uuid);
			strcpy(tp->SatListName, t->SatelliteListName);
			return true;
		}
	}
	return false;
}
bool cPluginMcli::TunerPoolDel(tuner_pool_t *tp)
{
	if(tp->type != -1) {
		tp->type=-1;
		return true;
	}
	return false;
}

tuner_pool_t *cPluginMcli::TunerAvailable(fe_type_t type, int pos, bool lock)
{
	tuner_pool_t *tp;
	if(lock) {
		Lock();
	}
#ifdef DEBUG_RESOURCES
	DEBUG_MASK(DEBUG_BIT_RESOURCES,
	dsyslog("mcli::%s: Testing for tuner type %d pos %d", __FUNCTION__, type, pos);
	)
#endif
	if (TunerCountByType (type) == m_cmd.tuner_type_limit[type]) {

#ifdef DEBUG_RESOURCES
		DEBUG_MASK(DEBUG_BIT_RESOURCES,
		dsyslog("mcli::%s: type %d limit (%d) reached", __FUNCTION__, type, m_cmd.tuner_type_limit[type]);
		)
#endif
		if(lock) {
			Unlock();
		}

		return NULL;
	}

	for(int i=0; i<TUNER_POOL_MAX; i++) {
		tp=m_tuner_pool+i;

		if(tp->inuse) {
			continue;
		}
		if(tp->type != type) {
			continue;
		}
#ifdef DEBUG_RESOURCES
		DEBUG_MASK(DEBUG_BIT_RESOURCES,
                dsyslog("mcli::%s: Tuner %d(%p), type %d, inuse %d", __FUNCTION__, i, tp, tp->type, tp->inuse);
		)
#endif
		if(TunerSatelitePositionLookup(tp, pos)) {
			if(lock) {
				Unlock();
			}
#ifdef DEBUG_RESOURCES
			DEBUG_MASK(DEBUG_BIT_RESOURCES,
		        dsyslog("mcli::%s: Tuner %d(%p) available", __FUNCTION__, i, tp);
			)
#endif

			return tp;
		}
	}
	if(lock) {
		Unlock();
	}

        esyslog("mcli::%s: No tuner available\n", __FUNCTION__);

	return NULL;
}

tuner_pool_t *cPluginMcli::TunerAlloc(fe_type_t type, int pos, bool lock)
{
	tuner_pool_t *tp;
	if(lock) {
		Lock();
	}
	tp=TunerAvailable(type, pos, false);
	if(tp) {
		tp->inuse=true;
#ifdef DEBUG_RESOURCES
		DEBUG_MASK(DEBUG_BIT_RESOURCES,
		dsyslog("mcli::%s: %p [%s], Type %d", __FUNCTION__, tp, tp->uuid, tp->type);
		)
#endif
		if(lock) {
			Unlock();
		}
		return tp;
	}
		if(lock) {
			Unlock();
		}
	return NULL;
}
bool cPluginMcli::TunerFree(tuner_pool_t *tp, bool lock)
{
	if(lock) {
		Lock();
	}
	if(tp->inuse) {
		tp->inuse=false;
#ifdef DEBUG_RESOURCES
		DEBUG_MASK(DEBUG_BIT_RESOURCES,
		dsyslog("mcli::%s: %p [%s], Type %d", __FUNCTION__, tp, tp->uuid, tp->type);
		)
#endif
		if(lock) {
			Unlock();
		}
		return true;
	}
	if(lock) {
		Unlock();
	}
	return false;
}

void cPluginMcli::Action (void)
{
	netceiver_info_list_t *nc_list = nc_get_list ();
//	printf ("Looking for netceivers out there....\n");
	bool channel_switch_ok = false;

#define NOTIFY_CAM_CHANGE 1
#ifdef NOTIFY_CAM_CHANGE
    int cam_stats[CAM_POOL_MAX] = { 0 };
    char menu_strings[CAM_POOL_MAX][MAX_MENU_STR_LEN];
    bool first_run = true;

    for (int i = 0; i < CAM_POOL_MAX; i++)
        menu_strings[i][0] = '\0';
#endif
    /** lets inform vdr and its plugins if TunerChange event happened */
	bool netCVChanged;

	while (Running ()) {
		netCVChanged = false;
		Lock ();
		if(!InitMcli()) {
			usleep (250 * 1000);
			continue;
		}
		nc_lock_list ();
		time_t now = time (NULL);
		bool tpa = false;

		for (int n = 0; n < nc_list->nci_num; n++) {
			netceiver_info_t *nci = nc_list->nci + n;

			//printf("cPluginMcli::Action: NCI Cam_Num: %d\n", nci->cam_num);

			if ((now - nci->lastseen) > MCLI_DEVICE_TIMEOUT) {
				if(CAMPoolDel(nci->uuid)) {
					isyslog  ("mcli::%s: Remove CAMs from NetCeiver: [%s]\n", __FUNCTION__, nci->uuid);
					netCVChanged = true;
				}
			} else {
				int cpa = CAMPoolAdd(nci);
				if(cpa==1) {
					isyslog ("mcli::%s: Add CAMs from NetCeiver: [%s] -> %d\n", __FUNCTION__, nci->uuid, cpa);
					netCVChanged = true;
				}
			}

                        if (netCVChanged) {
				for(int j = 0; j < nci->cam_num; j++) {

#ifdef DEBUG_RESOURCES
					DEBUG_MASK(DEBUG_BIT_RESOURCES,
					const char *camstate = "";
					const char *cammode = "";
					switch(nci->cam[j].status) {
						case DVBCA_CAMSTATE_MISSING: 
							camstate="MISSING"; break;
						case DVBCA_CAMSTATE_INITIALISING:
							camstate="INIT"; break;
						case DVBCA_CAMSTATE_READY:
							camstate="READY"; break;
					}
					switch(nci->cam[j].flags) {
						case CA_SINGLE:
							cammode="CA_SINGLE"; break;
						case CA_MULTI_SID:
							cammode="CA_MULTI_SID"; break;
						case CA_MULTI_TRANSPONDER:
							cammode="CA_MULTI_TRANSPONDER"; break;
					}

					if (nci->cam[j].status != DVBCA_CAMSTATE_MISSING) {
						dsyslog("mcli::%s: Slot:%d CamModule '%s' State:%s Mode:%s\n", __FUNCTION__, j, nci->cam[j].menu_string, camstate, cammode);
					} else {
						dsyslog("mcli::%s: Slot:%d CamModule State:%s\n", __FUNCTION__, j, camstate);
					}
					)
#endif
				}
			}



#if NOTIFY_CAM_CHANGE
            if (n == 0) {
                for(int j = 0; j < nci->cam_num && j < CAM_POOL_MAX; j++) {
                    if (nci->cam[j].status != cam_stats[j]) {
                        char buf[64];
                        if (nci->cam[j].status) {
                            if(nci->cam[j].status == 2 && !first_run) {
                                snprintf(buf, 64, tr("Module '%s' ready"), nci->cam[j].menu_string);
                                Skins.QueueMessage(mtInfo, buf);
                            }
                            cam_stats[j] = nci->cam[j].status;
                            strncpy(menu_strings[j], nci->cam[j].menu_string, MAX_MENU_STR_LEN);
                        } else if (nci->cam[j].status == 0) {
                            cam_stats[j] = nci->cam[j].status;
                            if (!first_run) {
                                snprintf(buf, 64, tr("Module '%s' removed"), (char*)menu_strings[j]);
                                Skins.QueueMessage(mtInfo, buf);
                            }
                            menu_strings[j][0] = '\0';
                        }
                    }
                }
                first_run = false;
            }
#endif

			for (int i = 0; i < nci->tuner_num; i++) {
				tuner_pool_t *t = TunerFindByUUID (nci->tuner[i].uuid);
				if (((now - nci->lastseen) > MCLI_DEVICE_TIMEOUT) || (nci->tuner[i].preference < 0) || !strlen (nci->tuner[i].uuid)) {
					if (t) {
						int pos=TunerPoolDel(t);
						isyslog  ("mcli::%s: Remove Tuner(#%d) %s [%s] @ %d\n", __FUNCTION__, i, nci->tuner[i].fe_info.name, nci->tuner[i].uuid, pos);
						//isyslog ("cPluginMcli::Action: Remove Tuner %s [%s] @ %d", nci->tuner[i].fe_info.name, nci->tuner[i].uuid, pos);
						netCVChanged = true;
					}
					continue;
				}
				if (!t) {
					tpa=TunerPoolAdd(nci->tuner+i);
					isyslog ("mcli::%s: Add Tuner(#%d): %s [%s], Type %d @ %d\n", __FUNCTION__, i, nci->tuner[i].fe_info.name, nci->tuner[i].uuid, nci->tuner[i].fe_info.type, tpa);
					//isyslog ("cPluginMcli::Action: Add Tuner: %s [%s], Type %d @ %d", nci->tuner[i].fe_info.name, nci->tuner[i].uuid, nci->tuner[i].fe_info.type, tpa);
					netCVChanged = true;
				}
			}
		}
		nc_unlock_list ();
		Unlock ();
		UpdateDevices();

		if (netCVChanged) {
			//RC: disabled, see mantis #995
			//cPluginManager::CallAllServices("NetCeiver changed");
		}
        
//TB: reelvdr itself tunes if the first tuner appears, don't do it twice
#if 1 //ndef REELVDR
		if (tpa && (m_debugmask & DEBUG_BIT_Action_NoRetuneFirstTuner)) {
			if (!channel_switch_ok) {	// the first tuner that was found, so make VDR retune to the channel it wants...
#if VDRVERSNUM < 20400
				cChannel *ch = Channels.GetByNumber (cDevice::CurrentChannel ());
#else
				LOCK_CHANNELS_READ;
				const cChannel *ch = Channels->GetByNumber (cDevice::CurrentChannel ());
#endif
				if (ch) {
#ifdef DEBUG_TUNE
					DEBUG_MASK(DEBUG_BIT_TUNE,
					dsyslog("mcli::%s: cDevice::PrimaryDevice (%p)", __FUNCTION__, cDevice::PrimaryDevice ());
					)
#endif
					channel_switch_ok = cDevice::PrimaryDevice ()->SwitchChannel (ch, true);
				}
			}
		} else {
			channel_switch_ok = 0;
		}
#endif

#ifdef TEMP_DISABLE_DEVICE
		TempDisableDevices();
#endif
		usleep (250 * 1000);
	}
}
void cPluginMcli::TempDisableDevices(bool now)
{
		for (cMcliDeviceObject * d = m_devs.First (); d; d = m_devs.Next (d)) {
			d->d ()->SetTempDisable (now);
		}

}
bool cPluginMcli::Initialize (void)
{
	InitMcli ();
	return true;
}


bool cPluginMcli::Start (void)
{
	isyslog("mcli version " MCLI_PLUGIN_VERSION " started");
#ifdef REELVDR
    if (access("/dev/dvb/adapter0", F_OK) != 0) //TB: this line allows the client to be used with usb-sticks without conflicts
#endif
	cThread::Start ();
	// Start any background activities the plugin shall perform.
	return true;
}

void cPluginMcli::Stop (void)
{
	cThread::Cancel (0);
	for (cMcliDeviceObject * d = m_devs.First (); d; d = m_devs.Next (d)) {
		d->d ()->SetEnable (false);
	}
	// Stop any background activities the plugin is performing.
}

void cPluginMcli::Housekeeping (void)
{
}

void cPluginMcli::MainThreadHook (void)
{
	if (reconf) {
		reconfigure ();
		reconf = 0;
	}
#if 0
	cOsdObject *MyMenu = AltMenuAction ();
	if (MyMenu) {		// is there any cam-menu waiting?
		if (cControl::Control ()) {
			cControl::Control ()->Hide ();
		}
		MyMenu->Show ();
	}
#endif
}

cString cPluginMcli::Active (void)
{
	// Return a message string if shutdown should be postponed
	return NULL;
}

time_t cPluginMcli::WakeupTime (void)
{
	// Return custom wakeup time for shutdown script
	return 0;
}

void cPluginMcli::reconfigure (void)
{
	Lock();
	for (cMcliDeviceObject * d = m_devs.First (); d;) {
		cMcliDeviceObject *next = m_devs.Next (d);
		d->d ()->SetEnable (false);
		d->d ()->ExitMcli ();
		d = next;
	}
	ExitMcli ();
	memset (m_tuner_pool, 0, sizeof(tuner_pool_t)*TUNER_POOL_MAX);
	for(int i=0; i<TUNER_POOL_MAX; i++) {
		m_tuner_pool[i].type = -1;
	}
	for(int i=0; i<CAM_POOL_MAX; i++) {
		m_cam_pool[i].max = -1;
	}
	InitMcli ();
	for (cMcliDeviceObject * d = m_devs.First (); d; d = m_devs.Next (d)) {
		d->d ()->InitMcli ();
	}
	Unlock();
	usleep(3*1000*1000);
	UpdateDevices();
}

void cPluginMcli::UpdateDevices() {
	int tc = TunerCount();
	int dc = min(tc, m_devs.Count());
	int c = dc;
	for (cMcliDeviceObject * d = m_devs.First (); d; d = m_devs.Next (d)) {
		if(c>0) {
			if(!d->d ()->HasInput())
				d->d ()->SetEnable(true);
			c--;
		} else if(d->d ()->HasInput())
			if(!d->d ()->Receiving())
				d->d ()->SetEnable(false);
	}
	static int last_dc=0;
	if(last_dc != dc) isyslog("%d tuner available: enabling %d devices", tc, dc);
	last_dc = dc;
}

cOsdObject *cPluginMcli::MainMenuAction (void)
{
	// Perform the action when selected from the main VDR menu.
	return new cCamMenu (&m_cmd);
}


cMenuSetupPage *cPluginMcli::SetupMenu (void)
{
	// Return a setup menu in case the plugin supports one.
	return new cMenuSetupMcli (&m_cmd);
}

bool cPluginMcli::SetupParse (const char *Name, const char *Value)
{
	if (!strcasecmp (Name, "DVB-C") && m_cmd.tuner_type_limit[FE_QAM] == MCLI_MAX_DEVICES)
		m_cmd.tuner_type_limit[FE_QAM] = atoi (Value);
	else if (!strcasecmp (Name, "DVB-T") && m_cmd.tuner_type_limit[FE_OFDM] == MCLI_MAX_DEVICES)
		m_cmd.tuner_type_limit[FE_OFDM] = atoi (Value);
	else if (!strcasecmp (Name, "DVB-S") && m_cmd.tuner_type_limit[FE_QPSK] == MCLI_MAX_DEVICES)
		m_cmd.tuner_type_limit[FE_QPSK] = atoi (Value);
	else if (!strcasecmp (Name, "DVB-S2") && m_cmd.tuner_type_limit[FE_DVBS2] == MCLI_MAX_DEVICES)
		m_cmd.tuner_type_limit[FE_DVBS2] = atoi (Value);
	else
		return false;
	return true;
}

bool cPluginMcli::Service (const char *Id, void *Data)
{
	mclituner_info_t *infos = (mclituner_info_t *) Data;

	if (Id && strcmp (Id, "GetTunerInfo") == 0) {
		int j=0;
		time_t now = time (NULL);
		netceiver_info_list_t *nc_list = nc_get_list ();
		nc_lock_list ();
		for (int n = 0; n < nc_list->nci_num; n++) {
			netceiver_info_t *nci = nc_list->nci + n;
			if ((now - nci->lastseen) > MCLI_DEVICE_TIMEOUT) {
				continue;
			}
			for (int i = 0; i < nci->tuner_num && j < MAX_TUNERS_IN_MENU; i++) {
				strcpy (infos->name[j], nci->tuner[i].fe_info.name);
				infos->type[j] = nci->tuner[i].fe_info.type;
				infos->preference[j++] = nci->tuner[i].preference;
#ifdef DEBUG_TUNE
				DEBUG_MASK(DEBUG_BIT_TUNE,
				dsyslog("mcli::%s: Tuner: %s", __FUNCTION__, nci->tuner[i].fe_info.name);
				)
#endif
			}
		}
		nc_unlock_list ();
		return true;
	} else if (Id && strcmp (Id, "Reinit") == 0) {
		if (Data && strlen ((char *) Data) && (strncmp ((char *) Data, "eth", 3) || strncmp ((char *) Data, "br", 2))) {
			strncpy (m_cmd.iface, (char *) Data, IFNAMSIZ - 1);
		}
		reconfigure ();
		return true;
	} else if (Id && strcmp (Id, "Set tuner count") == 0) {
		if (Data) {
			  mcli_tuner_count_t *tuner_count = (mcli_tuner_count_t*)Data;
			int count;

			count = tuner_count->dvb_c;
			if (count < 0) count = MCLI_MAX_DEVICES;
			//SetupParse("DVB-C", itoa(count));
			m_cmd.tuner_type_limit[FE_QAM] = count;
			/* save settings to .conf*/
			SetupStore("DVB-C", count);

			count = tuner_count->dvb_t;
			if (count < 0) count = MCLI_MAX_DEVICES;
			//SetupParse("DVB-T", itoa(count));
			m_cmd.tuner_type_limit[FE_OFDM] = count;
			/* save settings to .conf*/
			SetupStore("DVB-T", count);

			count = tuner_count->dvb_s;
			if (count < 0) count = MCLI_MAX_DEVICES;
			//SetupParse("DVB-S", itoa(count));
			m_cmd.tuner_type_limit[FE_QPSK] = count;
			/* save settings to .conf*/
			SetupStore("DVB-S", count);

			count = tuner_count->dvb_s2;
			if (count < 0) count = MCLI_MAX_DEVICES;
			//SetupParse("DVB-S2", itoa(count));
			m_cmd.tuner_type_limit[FE_DVBS2] = count;
			/* save settings to .conf*/
			SetupStore("DVB-S2", count);
		}
		return true;
	} // set tuner count
	else if (Id && strcmp (Id, "Get tuner count") == 0) {
		if (Data) {
			mcli_tuner_count_t *tuner_count = (mcli_tuner_count_t*)Data;

			tuner_count->dvb_c = TunerCountByType(FE_QAM);
			tuner_count->dvb_t = TunerCountByType(FE_OFDM);
			tuner_count->dvb_s = TunerCountByType(FE_QPSK);
			tuner_count->dvb_s2 = TunerCountByType((fe_type_t)FE_DVBS2);
		}
		return true;
	}
	// Handle custom service requests from other plugins
	return false;
}

const char **cPluginMcli::SVDRPHelpPages (void)
{
	// Return help text for SVDRP commands this plugin implements
	static const char *HelpPages[] = {
                "GETTC\n"        "    List available tuners.",
		"REINIT [dev]\n" "    Reinitalize the plugin on a certain network device - e.g.: plug mcli REINIT eth0",
		NULL
	};
	return HelpPages;
}

cString cPluginMcli::SVDRPCommand (const char *Command, const char *Option, int &ReplyCode)
{
        typedef struct nrTuners
        {
            int sat;
            int satS2;
            int cable;
            int terr;
        } nrTuners_t;

	// Process SVDRP commands this plugin implements

	if (strcasecmp (Command, "REINIT") == 0) {
		if (Option && (strncmp (Option, "eth", 3) || strncmp (Option, "br", 2))) {
			strncpy (m_cmd.iface, (char *) Option, IFNAMSIZ - 1);
		}
		reconfigure ();
		return cString ("Mcli-plugin: reconfiguring...");
	}
        else if (strcasecmp(Command, "GETTC") == 0)
        {
	    std::stringstream sdat;
            std::string sout;

            char *buffer = NULL;
            std::string strBuff;
            FILE *file = NULL;

	    int cable = 9999;
	    int sat   = 9999;
            int satS2 = 9999;
            int terr  = 9999;
            file = fopen("/etc/default/mcli", "r");
            if(file)
            {
                cReadLine readline;
                buffer = readline.Read(file);
                while(buffer)
                {
                    if(strstr(buffer, "DVB_C_DEVICES=\"") && !strstr(buffer, "\"\""))
                        cable = atoi(buffer+15);
                    if(strstr(buffer, "DVB_S_DEVICES=\"") && !strstr(buffer, "\"\""))
                        sat   = atoi(buffer+15);
                    if(strstr(buffer, "DVB_S2_DEVICES=\"") && !strstr(buffer, "\"\""))
                        satS2 = atoi(buffer+16);
                    if(strstr(buffer, "DVB_T_DEVICES=\"") && !strstr(buffer, "\"\""))
                        terr  = atoi(buffer+15);

                    buffer = readline.Read(file);
                }
                fclose(file);
            }

            nrTuners_t nrTunersPhys;
            nrTunersPhys.sat = nrTunersPhys.satS2 = nrTunersPhys.terr = nrTunersPhys.cable = 0;
            cPlugin *mcliPlugin = cPluginManager::GetPlugin("mcli");
            if (mcliPlugin)
            {
                mclituner_info_t info;
                for (int i = 0; i < MAX_TUNERS_IN_MENU; i++)
                    info.name[i][0] = '\0';
                mcliPlugin->Service("GetTunerInfo", &info);
                for (int i = 0; i < MAX_TUNERS_IN_MENU; i++)
                {
                    if (info.preference[i] == -1 || strlen(info.name[i]) == 0)
                        break;
                    else
                    {
                        switch(info.type[i])
                        {
                            case FE_QPSK: // DVB-S
                                nrTunersPhys.sat++;
                                break;
                            case FE_DVBS2: // DVB-S2
                                nrTunersPhys.satS2++;
                                break;
                            case FE_OFDM: // DVB-T
                                nrTunersPhys.terr++;
                                break;
                            case FE_QAM: // DVB-C
                                nrTunersPhys.cable++;
                                break;
                        }
                    }
                }
            }

	    if ( cable > nrTunersPhys.cable )
                cable = nrTunersPhys.cable;
            if ( sat   > nrTunersPhys.sat )
		sat   = nrTunersPhys.sat;
            if ( satS2 > nrTunersPhys.satS2 )
		satS2  = nrTunersPhys.satS2;
	    if ( terr  > nrTunersPhys.terr )
		terr   = nrTunersPhys.terr;

	    sdat.str("");
            if ( asprintf( &buffer, "DVB_C_DEVICES=%d\n", cable ) >= 0 )
	    {
		sdat.str(""); sdat << buffer;
		sout += sdat.str();
		free(buffer);
	    }

            if ( asprintf( &buffer, "DVB_S_DEVICES=%d\n", sat ) >= 0 )
	    {
		sdat.str(""); sdat << buffer;
		sout += sdat.str();
		free(buffer);
	    }

            if ( asprintf( &buffer, "DVB_S2_DEVICES=%d\n", satS2 ) >= 0 )
	    {
		sdat.str(""); sdat << buffer;
		sout += sdat.str();
		free(buffer);
	    }

            if ( asprintf( &buffer, "DVB_T_DEVICES=%d\n", terr ) >= 0 )
	    {
		sdat.str(""); sdat << buffer;
		sout += sdat.str();
		free(buffer);
	    }
	    ReplyCode = 215;
            return cString( sout.c_str() );
        }
	return NULL;
}

VDRPLUGINCREATOR (cPluginMcli);	// Don't touch this!
