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

#define MCLI_DEVICE_VERSION "0.9.3"
#define MCLI_PLUGIN_VERSION "0.9.3"
#define MCLI_PLUGIN_DESCRIPTION trNOOP ("NetCeiver Client Application")
#define MCLI_SETUPMENU_DESCRIPTION trNOOP ("NetCeiver Client Application")
#define MCLI_MAINMENU_DESCRIPTION trNOOP ("Common Interface")

#define MCLI_MAX_DEVICES 8
#define MCLI_DEVICE_TIMEOUT 120

#define TUNER_POOL_MAX 32
#define CAM_POOL_MAX 10

//#define TEMP_DISABLE_DEVICE
#define TEMP_DISABLE_TIMEOUT_DEFAULT (10)
#define TEMP_DISABLE_TIMEOUT_SCAN (30)
#define TEMP_DISABLE_TIMEOUT_CAOVERRIDE (30)
#define LASTSEEN_TIMEOUT (10)
//#define ENABLE_DEVICE_PRIORITY

//#define DEBUG_PIDS 
//#define DEBUG_TUNE_EXTRA
//#define DEBUG_TUNE
//#define DEBUG_RESOURCES

class cMcliDeviceObject:public cListObject
{
      public:
	cMcliDeviceObject (cMcliDevice * d)
	{
		m_d = d;
	}
	 ~cMcliDeviceObject (void)
	{
	}
	cMcliDevice *d (void)
	{
		return m_d;
	}
      private:
	cMcliDevice * m_d;
};

class cMcliDeviceList:public cList < cMcliDeviceObject >
{
      public:
	cMcliDeviceList (void)
	{
	};
	~cMcliDeviceList () {
		dsyslog ("Delete my Dev list\n");
	};
};

typedef struct tuner_pool {
	int type;
	char uuid[UUID_SIZE+1];
	char SatListName[UUID_SIZE+1];
	bool inuse;
} tuner_pool_t;

typedef struct cam_pool {
	char uuid[UUID_SIZE+1];
	int slot;
	int use;
	int max;
	int status;
} cam_pool_t;

class cPluginMcli:public cPlugin, public cThread
{
      private:
	// Add any member variables or functions you may need here.
	cMcliDeviceList m_devs;
	cmdline_t m_cmd;
	UDPContext *m_cam_mmi;
	cam_pool_t m_cam_pool[CAM_POOL_MAX]; 
	int m_mmi_init_done;
	int m_recv_init_done;
	int m_mld_init_done;
	int m_api_init_done;
        bool m_cam_present;
	tuner_pool_t m_tuner_pool[TUNER_POOL_MAX];
	tuner_pool_t *TunerAvailableInt(fe_type_t type, int pos);
	
      public:
	cPluginMcli (void);
	virtual ~ cPluginMcli ();
	virtual const char *Version (void)
	{
		return MCLI_PLUGIN_VERSION;
	}
	virtual const char *Description (void)
	{
		return MCLI_PLUGIN_DESCRIPTION;
	}
	virtual const char *CommandLineHelp (void);
	virtual bool ProcessArgs (int argc, char *argv[]);
	virtual bool Initialize (void);
	virtual bool Start (void);
	virtual void Stop (void);
	virtual void Housekeeping (void);
	virtual void MainThreadHook (void);
	virtual cString Active (void);
	virtual time_t WakeupTime (void);
#ifdef REELVDR
	virtual bool HasSetupOptions (void)
	{
		return false;
	}
#endif
	virtual const char *MenuSetupPluginEntry (void)
	{
#ifdef REELVDR
		return NULL;
#else
		return MCLI_SETUPMENU_DESCRIPTION;
#endif
	}
	virtual const char *MainMenuEntry (void)
	{
		return MCLI_MAINMENU_DESCRIPTION;
	}
	virtual cOsdObject *MainMenuAction (void);
	virtual cMenuSetupPage *SetupMenu (void);
	virtual bool SetupParse (const char *Name, const char *Value);
	virtual bool Service (const char *Id, void *Data = NULL);
	virtual const char **SVDRPHelpPages (void);
	virtual cString SVDRPCommand (const char *Command, const char *Option, int &ReplyCode);
	virtual void Action (void);
	
	void ExitMcli (void);
	bool InitMcli (void);
	void reconfigure (void);
	void UpdateDevices();

	int CAMPoolAdd(netceiver_info_t *nci);
	bool CAMPoolDel(const char *uuid);
	cam_pool_t *CAMPoolFindFree(void);
	cam_pool_t *CAMFindByUUID (const char *uuid, int slot=-1);
	cam_pool_t *CAMAvailable (const char *uuid=NULL, int slot=-1, bool lock=true);
	cam_pool_t *CAMAlloc (const char *uuid=NULL, int slot=-1);
	int CAMFree (cam_pool_t *cp);
	bool CAMSteal(const char *uuid=NULL, int slot=-1, bool force=false);
	bool CAMPresent() { return m_cam_present; }

	satellite_list_t *TunerFindSatList(const netceiver_info_t *nc_info, const char *SatelliteListName) const;
	bool SatelitePositionLookup(const satellite_list_t *satlist, int pos) const;
	bool TunerSatelitePositionLookup(tuner_pool_t *tp, int pos) const;
	
	tuner_pool_t *TunerFindByUUID (const char *uuid);
	bool Ready();
	int TunerCount();
	int TunerCountByType (const fe_type_t type);
	bool TunerPoolAdd(tuner_info_t *t);
	bool TunerPoolDel(tuner_pool_t *tp);
	tuner_pool_t *TunerAvailable(fe_type_t type, int pos, bool lock=true);
	tuner_pool_t *TunerAlloc(fe_type_t type, int pos, bool lock=true);
	bool TunerFree(tuner_pool_t *tp, bool lock=true);

	int CamPollText (mmi_info_t * text);
	void TempDisableDevices(bool now=false);
	
	virtual cOsdObject *AltMenuAction (void);
};
