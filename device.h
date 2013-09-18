/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#ifndef VDR_MCLI_DEVICE_H
#define VDR_MCLI_DEVICE_H

#include <vdr/device.h>
#include <mcliheaders.h>

#include "packetbuffer.h"

class cPluginMcli;
struct tuner_pool;
struct cam_pool;

class cMcliDevice:public cDevice
{

      private:
	int m_pidsnum;
	int m_mcpidsnum;
	bool m_dvr_open;
	recv_info_t *m_r;
	recv_sec_t m_sec;
	int m_pos;
	struct dvb_frontend_parameters m_fep;
	dvb_pid_t m_pids[RECV_MAX_PIDS];
	tra_t m_ten;
	int m_fetype;
	cChannel m_chan;
	cMutex mutex;
	bool m_enable;
	time_t m_last;
	int m_filternum;
	int m_disabletimeout;
	bool m_tuned;
	bool m_showtuning;
	bool m_ca_enable;
	bool m_ca_override;
	struct tuner_pool *m_tunerref;
	struct cam_pool *m_camref;

      protected:
      	cPluginMcli *m_mcli;
	virtual bool SetChannelDevice (const cChannel * Channel, bool LiveView);
	virtual bool HasLock(int TimeoutMs = 0) const;

	virtual bool SetPid (cPidHandle * Handle, int Type, bool On);
	virtual bool OpenDvr (void);
	virtual void CloseDvr (void);
	virtual bool GetTSPacket (uchar * &Data);

	virtual int OpenFilter (u_short Pid, u_char Tid, u_char Mask);
	virtual void CloseFilter (int Handle);
	virtual bool CheckCAM(const cChannel * Channel, bool steal=false) const;

#ifdef GET_TS_PACKETS
	virtual int GetTSPackets (uchar *, int);
#endif
	void TranslateTypePos(int &type, int &pos, const int Source) const;
	
	
      public:
	cCondVar m_locked;
#ifdef USE_VDR_PACKET_BUFFER
	cRingBufferLinear *m_PB;
	cMutex m_PB_Lock;
	bool delivered;
#else
	cMyPacketBuffer *m_PB;
#endif
	cMcliFilters *m_filters;
	cMcliDevice (void);
	virtual ~ cMcliDevice ();
	virtual bool Ready();
	void SetMcliRef(cPluginMcli *m)
	{
		m_mcli=m;
	}
	virtual int NumProvidedSystems(void) const
	{
		return (m_fetype == FE_DVBS2)?2:1;
	}
  
#ifdef REELVDR
	const cChannel *CurChan () const
	{
		return &m_chan;
	};
#endif
	unsigned int FrequencyToHz (unsigned int f)
	{
		while (f && f < 1000000)
			f *= 1000;
		return f;
	}
	virtual bool HasInternalCam (void)
	{
		return true; 
	}
	virtual bool ProvidesSource (int Source) const;
	virtual bool ProvidesTransponder (const cChannel * Channel) const;
	virtual bool ProvidesChannel (const cChannel * Channel, int Priority = -1, bool * NeedsDetachReceivers = NULL) const;
	virtual bool IsTunedToTransponder (const cChannel * Channel) const;

	virtual int HandleTsData (unsigned char *buffer, size_t len);
	tra_t *GetTenData (void) {
		return &m_ten;
	}
	void SetTenData (tra_t * ten);
	void SetCaEnable (bool val = true) 
	{
		m_ca_enable=val;
	}
	bool GetCaEnable (void) const
	{
		return m_ca_enable;
	}
	struct cam_pool *GetCAMref (void) const
	{
		return m_camref;
	}
	void SetCaOverride (bool val = true) 
	{
		m_ca_override=val;
	}
	bool GetCaOverride (void) const
	{
		return m_ca_override;
	}
	void SetEnable (bool val = true);
	bool SetTempDisable (bool now = false);
	void SetFEType (fe_type_t val);
	fe_type_t GetFEType (void)
	{
		return (fe_type_t)m_fetype;
	};
	void InitMcli (void);
	void ExitMcli (void);
	virtual bool ProvidesS2 () const
	{
		return m_fetype == FE_DVBS2;
	}
	virtual bool HasInput (void) const
	{
		return m_enable;
	}
#ifdef DEVICE_ATTRIBUTES
	// Reel extension
	virtual int GetAttribute (const char *attr_name, uint64_t * val);
	virtual int GetAttribute (const char *attr_name, char *val, int maxret);
#endif
#if VDRVERSNUM > 10720
	bool ProvidesEIT(void) const {
		return true;
	}
#endif
       virtual int SignalStrength(void) const;
       virtual int SignalQuality(void) const;
};

#endif // VDR_MCLI_DEVICE_H
