/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#include <time.h>
#include <iostream>

#include <vdr/channels.h>
#include <vdr/ringbuffer.h>
#include <vdr/eit.h>
#include <vdr/timers.h>
#include <vdr/skins.h>
#include <vdr/eitscan.h>

#include "filter.h"
#include "device.h"
#include "mcli.h"

#define st_Pos  0x07FF
#define st_Neg  0x0800

#define DVB_SYSTEM_1 0
#define DVB_SYSTEM_2 1

using namespace std;

static int handle_ts (unsigned char *buffer, size_t len, void *p)
{
	return p ? ((cMcliDevice *) p)->HandleTsData (buffer, len) : len;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static int handle_ten (tra_t * ten, void *p)
{
	cMcliDevice *m = (cMcliDevice *) p;
	if (ten) {
//              fprintf (stderr, "Status:%02X, Strength:%04X, SNR:%04X, BER:%04X\n", ten->s.st, ten->s.strength, ten->s.snr, ten->s.ber);
		m->SetTenData (ten);
		if (ten->s.st & FE_HAS_LOCK) {
			m->m_locked.Broadcast ();
		}
	} else {
		tra_t ten;
		memset (&ten, 0, sizeof (tra_t));
		m->SetTenData (&ten);
//              fprintf (stderr, "Signal lost\n");
	}
	return 0;
}

cMcliDevice::cMcliDevice (void)
{
	m_enable = false;
	m_tuned = false;
	StartSectionHandler ();
#ifdef USE_VDR_PACKET_BUFFER
	//printf ("mcli::%s: USING VDR PACKET BUFFER \n", __FUNCTION__);
	m_PB = new cRingBufferLinear(MEGABYTE(4), TS_SIZE, false, "MCLI_TS");
	m_PB->SetTimeouts (0, 100);
	delivered = false;
#else
	//printf ("mcli::%s: USING INTERNAL MCLI PACKET BUFFER \n", __FUNCTION__);
	m_PB = new cMyPacketBuffer (10000 * TS_SIZE, 10000);
	m_PB->SetTimeouts (0, CLOCKS_PER_SEC * 20 / 1000);
#endif
	m_filters = new cMcliFilters ();

	m_pidsnum = 0;
	m_mcpidsnum = 0;
	m_filternum = 0;
	m_mcli = NULL;
	m_fetype = -1;
	m_last = 0;
	m_showtuning = 0;
	m_ca_enable = false;
	m_ca_override = false;
	memset (m_pids, 0, sizeof (m_pids));
	memset (&m_ten, 0, sizeof (tra_t));
	m_pids[0].pid=-1;
	m_disabletimeout = TEMP_DISABLE_TIMEOUT_DEFAULT;
	m_tunerref = NULL;
	m_camref = NULL;
	InitMcli ();

#ifdef DEBUG_RESOURCES
	DEBUG_MASK(DEBUG_BIT_RESOURCES,
	dsyslog ("mcli::%s: DVB got device number %d", __FUNCTION__, CardIndex () + 1);
	)
#endif

}

cMcliDevice::~cMcliDevice ()
{
	LOCK_THREAD;
	StopSectionHandler ();
#ifdef DEBUG_RESOURCES
	DEBUG_MASK(DEBUG_BIT_RESOURCES,
	dsyslog ("mcli::%s: DVB %d gets destructed", __FUNCTION__, CardIndex () + 1);
	)
#endif
	Cancel (0);
	m_locked.Broadcast ();
	ExitMcli ();
	DELETENULL (m_filters);
	DELETENULL (m_PB);
}

bool cMcliDevice::Ready() {
	return m_mcli ? m_mcli->Ready() : false;
}

void cMcliDevice::SetTenData (tra_t * ten)
{
	if(!ten->lastseen) {
		ten->lastseen=m_ten.lastseen;
	}
	m_ten = *ten;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void cMcliDevice::SetEnable (bool val)
{
	LOCK_THREAD;
#ifdef DEBUG_TUNE
	DEBUG_MASK(DEBUG_BIT_TUNE,
	dsyslog("mcli::%s: device %d: %d -> %d", __FUNCTION__, CardIndex()+1, m_enable, val);
	)
#endif
	m_enable = val;
	if (!m_enable) {
		recv_stop (m_r);
		m_tuned = false;
		if(GetCaEnable()) {
			SetCaEnable(false);
			m_mcli->CAMFree(m_camref);
			m_camref = NULL;
		}
		if(m_tunerref) {
			m_mcli->TunerFree(m_tunerref);
			m_tunerref = NULL;
			m_fetype = -1;
		}
	} else {
		if(m_tunerref == NULL) {
#if VDRVERSNUM < 10702	
			bool s2=m_chan.Modulation() == QPSK_S2 || m_chan.Modulation() == PSK8;
#elif VDRVERSNUM < 10714
			bool s2=m_chan.System() == SYS_DVBS2;
#elif VDRVERSNUM < 10722
			cDvbTransponderParameters dtp(m_chan.Parameters());
			bool s2=dtp.System() == SYS_DVBS2;
#elif VDRVERSNUM < 10723
        #error "vdr version not supported"
#else
			cDvbTransponderParameters dtp(m_chan.Parameters());
			bool s2=dtp.System() == DVB_SYSTEM_2;
#endif
			bool ret = false;
			int pos;
			int type;
				
			TranslateTypePos(type, pos, m_chan.Source());
			if(s2) {
				type=FE_DVBS2;
			}
			ret = m_mcli->TunerAvailable((fe_type_t)type, pos);
			if(!ret && type == FE_QPSK) {
				type = FE_DVBS2;
				ret = m_mcli->TunerAvailable((fe_type_t)type, pos);
			}
			if(!ret) {
				return;
			}
			m_fetype = type;

			int slot = -1;
			if(m_chan.Ca(0)<=0xff) {
				slot=m_chan.Ca(0)&0x03;
				if(slot) {
					slot--;
				}
			}

			bool triggerCam = not(m_debugmask & DEBUG_BIT_Action_SkipTriggerCam);
			if((m_chan.Ca() || triggerCam) && !GetCaEnable() && m_mcli->CAMAvailable(NULL, slot) && (m_camref=m_mcli->CAMAlloc(NULL, slot))) {
				SetCaEnable();
			}

			recv_tune (m_r, (fe_type_t)m_fetype, m_pos, &m_sec, &m_fep, m_pids);
			m_tuned = true;
		}
	}
}

bool cMcliDevice::SetTempDisable (bool now)
{
	if(!now) {
		Lock();
	}
//#ifndef REELVDR // they might find it out in some other place
	// Check for tuning timeout
	if(m_showtuning && Receiving(true) && ((time(NULL)-m_ten.lastseen)>=LASTSEEN_TIMEOUT)) {
		Skins.QueueMessage(mtInfo, cString::sprintf(tr("Waiting for a free tuner (%s)"),m_chan.Name()));
		m_showtuning = false;
	}
//#endif
//	printf("Device %d Receiving %d Priority %d\n",CardIndex () + 1, Receiving (true), Priority());
	if((!Receiving (true) && (((time(NULL)-m_last) >= m_disabletimeout)) ) || now) {
		recv_stop (m_r);
		m_tuned = false;
		if(GetCaEnable()) {
			SetCaEnable(false);
#ifdef DEBUG_TUNE
			DEBUG_MASK(DEBUG_BIT_TUNE,
			dsyslog("mcli::%s: Releasing CAM on device %d (%s) (disable, %d)\n", __FUNCTION__, CardIndex()+1, m_chan.Name(), now);
			)
#endif
			m_mcli->CAMFree(m_camref);
			m_camref = NULL;
		}
		if(m_tunerref) {
#ifdef DEBUG_TUNE
			DEBUG_MASK(DEBUG_BIT_TUNE,
			dsyslog("mcli::%s: Releasing tuner on device %d (%s)", __FUNCTION__, CardIndex()+1, m_chan.Name());
			)
#endif			
			m_mcli->TunerFree(m_tunerref, false);
			m_tunerref = NULL;
			m_fetype = -1;
		}
		if(!now) {
			Unlock();
		}
		return true;
	}
	if(!now) {
		Unlock();
	}
	return false;
}

void cMcliDevice::SetFEType (fe_type_t val)
{
	m_fetype = (int)val;
}

int cMcliDevice::HandleTsData (unsigned char *buffer, size_t len)
{
#ifdef USE_VDR_PACKET_BUFFER
	cMutexLock lock (&m_PB_Lock);
#endif
	m_filters->PutTS (buffer, len);
#ifdef GET_TS_PACKETS
	unsigned char *ptr = m_PB->PutStart (len);
	if (ptr) {
		memcpy (ptr, buffer, len);
		m_PB->PutEnd (len, 0, 0);
	}
#else
#ifdef USE_VDR_PACKET_BUFFER
	if((size_t)m_PB->Free() < len) { // m_PB->Free() returns an unsigned int
		m_PB->Clear();
		if(Receiving(true))  isyslog("mcli::%s: buffer overflow [%d] %s", __FUNCTION__, CardIndex()+1, m_chan.Name());
	}
	return m_PB->Put(buffer, len);
#else
	unsigned int i;
	for (i = 0; i < len; i += TS_SIZE) {
		unsigned char *ptr = m_PB->PutStart (TS_SIZE);
		if (ptr) {
			memcpy (ptr, buffer + i, TS_SIZE);
			m_PB->PutEnd (TS_SIZE, 0, 0);
		}
	}
#endif
#endif
	return len;
}


void cMcliDevice::InitMcli (void)
{
	m_r = recv_add ();

	register_ten_handler (m_r, handle_ten, this);
	register_ts_handler (m_r, handle_ts, this);
}

void cMcliDevice::ExitMcli (void)
{
	register_ten_handler (m_r, NULL, NULL);
	register_ts_handler (m_r, NULL, NULL);
	recv_del (m_r);
	m_r = NULL;
}

bool cMcliDevice::ProvidesSource (int Source) const
{
	int pos;
	int type;
	bool ret=false;

	if (!m_enable) {
		return false;
	}
	
	TranslateTypePos(type, pos, Source);

	if(m_tunerref) {
		ret= (type == m_fetype) || (type == FE_QPSK && m_fetype == FE_DVBS2);
		if(ret) {
			ret = m_mcli->TunerSatelitePositionLookup(m_tunerref, pos);
		}
	}
	
	if(!ret) {
		ret = m_mcli->TunerAvailable((fe_type_t)type, pos);
		if(!ret && type == FE_QPSK) {
			type = FE_DVBS2;
			ret = m_mcli->TunerAvailable((fe_type_t)type, pos);
		}
	} 
#ifdef DEBUG_TUNE_EXTRA
	DEBUG_MASK(DEBUG_BIT_TUNE_EXTRA,
	dsyslog ("mcli::%s: DVB:%d Type:%d Pos:%d -> %d\n", __FUNCTION__, CardIndex () + 1, type, pos, ret);
	)
#endif
	return ret;
}

bool cMcliDevice::ProvidesTransponder (const cChannel * Channel) const
{
	if (!m_enable) {
		return false;
	}
 	
#if VDRVERSNUM < 10702	
	bool s2=Channel->Modulation() == QPSK_S2 || Channel->Modulation() == PSK8;
#elif VDRVERSNUM < 10714	
	bool s2=Channel->System() == SYS_DVBS2;
#elif VDRVERSNUM < 10722
	cDvbTransponderParameters dtp(Channel->Parameters());
	bool s2=dtp.System() == SYS_DVBS2;
#elif VDRVERSNUM < 10723
	#error "vdr version not supported"
#else
	cDvbTransponderParameters dtp(Channel->Parameters());
	bool s2=dtp.System() == DVB_SYSTEM_2;
#endif	
	bool ret=ProvidesSource (Channel->Source ());

        //printf ("mcli::%s: DEBUG 'Channel->Parameters()' -> (%u) \n", __FUNCTION__, Channel->Parameters() );
        //printf ("mcli::%s: DEBUG 's2=dtp.System() == DVB_SYSTEM_2' -> (%d)=(%d) == (%d) \n", __FUNCTION__, s2, dtp.System(), DVB_SYSTEM_2 );
        //printf ("mcli::%s: DEBUG 'Channel->Source ()' -> (%u) \n", __FUNCTION__, Channel->Source () );

	if(ret) {
		int pos;
		int type;
		TranslateTypePos(type, pos, Channel->Source());
		if(s2) {
			type=FE_DVBS2;
		}
		if(m_tunerref) {
			ret = (m_fetype == type) || (type == FE_QPSK && m_fetype == FE_DVBS2);
		} else {
			ret = false;
		}
		if(!ret) {
			ret = m_mcli->TunerAvailable((fe_type_t)type, pos);
			if(!ret && type == FE_QPSK) {
				type = FE_DVBS2;
				ret = m_mcli->TunerAvailable((fe_type_t)type, pos);
			}
		}
	}

#ifdef DEBUG_TUNE_EXTRA
	DEBUG_MASK(DEBUG_BIT_TUNE_EXTRA,
	dsyslog ("mcli::%s: DVB:%d S2:%d %s@%p -> %d", __FUNCTION__, CardIndex () + 1, s2, Channel->Name (), this, ret);
	)
#endif
	return ret;
}

bool cMcliDevice::IsTunedToTransponder(const cChannel * Channel) const
{
//      printf ("IsTunedToTransponder %s == %s \n", Channel->Name (), m_chan.Name ());
	if (!m_enable || !m_tuned) {
		return false;
	}

#if VDRVERSNUM > 10713
	 cDvbTransponderParameters m_dtp(m_chan.Parameters());
	 cDvbTransponderParameters dtp(Channel->Parameters());
#endif
 
	if (m_ten.s.st & FE_HAS_LOCK && m_chan.Source() == Channel->Source() &&
	        m_chan.Transponder() == Channel->Transponder() && m_chan.Frequency() == Channel->Frequency() &&
#if VDRVERSNUM > 10713
			m_dtp.Modulation() == dtp.Modulation() &&
#else	        
	                m_chan.Modulation() == Channel->Modulation() &&
#endif
	                        m_chan.Srate() == Channel->Srate()) {
		return true;
	}
	return false;
}


bool cMcliDevice::CheckCAM(const cChannel * Channel, bool steal) const
{
	if(GetCaOverride() || !Channel->Ca()) {
		return true;
	}
	int slot = -1;
	if(Channel->Ca(0)<=0xff) {
		slot=Channel->Ca(0)&0x03;
		if(slot) {
			slot--;
		}
	}
	if(m_camref && (m_camref->slot == slot || slot == -1)) {
		return true;
	} 
	if(!m_mcli->CAMAvailable(NULL, slot) && !m_mcli->CAMSteal(NULL, slot, steal)) {
		return false;
	}
	return true;
}
 
bool cMcliDevice::ProvidesChannel (const cChannel * Channel, int Priority, bool * NeedsDetachReceivers) const
{
	bool result = false;
	bool hasPriority = Priority < 0 || Priority > this->Priority ();
	bool needsDetachReceivers = false;
	if (!m_enable) {
		return false;
	}
	if(!CheckCAM(Channel, false)) {
#ifdef DEBUG_TUNE
		DEBUG_MASK(DEBUG_BIT_TUNE,
		dsyslog ("mcli::%s: DVB:%d Channel:%s, Prio:%d this->Prio:%d m_chan.Name:%s -> %d", __FUNCTION__, CardIndex () + 1, Channel->Name (), Priority, this->Priority (), m_chan.Name(), false);
		)
#endif
		return false;
	}
	if(ProvidesTransponder(Channel)) {
#ifdef DEBUG_TUNE_PC
		DEBUG_MASK(DEBUG_BIT_TUNE_PC,
		dsyslog ("mcli::ProvidesChannel: DEBUG DVB:%d Channel:%s * 'ProvidesTransponder(Channel)' is True", CardIndex () + 1, Channel->Name ());
		)
#endif
		result = hasPriority;

#ifdef DEBUG_TUNE_PC
		DEBUG_MASK(DEBUG_BIT_TUNE_PC,
		dsyslog ("mcli::ProvidesChannel: DEBUG result %d hasPriority %d", result, hasPriority);
		)
#endif

		if (Priority >= 0 && Receiving (true))
		{
#ifdef DEBUG_TUNE_PC
			DEBUG_MASK(DEBUG_BIT_TUNE_PC,
	                dsyslog ("mcli::ProvidesChannel: DEBUG DVB:%d Channel:%s * 'Priority >= 0 && Receiving (true)' is True", CardIndex () + 1, Channel->Name ());
			)
#endif

			if (!IsTunedToTransponder(Channel)) {
				needsDetachReceivers = true;
#ifdef DEBUG_TUNE_PC
				DEBUG_MASK(DEBUG_BIT_TUNE_PC,
	                        dsyslog ("mcli::ProvidesChannel: DEBUG DVB:%d Channel:%s * '!IsTunedToTransponder(Channel)' is True", CardIndex () + 1, Channel->Name ());
				)
#endif

			} else {
				result = true;
#ifdef DEBUG_TUNE_PC
				DEBUG_MASK(DEBUG_BIT_TUNE_PC,
                                dsyslog ("mcli::ProvidesChannel: DEBUG DVB:%d Channel:%s * '!IsTunedToTransponder(Channel)' is False * result = true ***** OK", CardIndex () + 1, Channel->Name ());
				)
#endif
			}

		} else {
#ifdef DEBUG_TUNE_PC
			DEBUG_MASK(DEBUG_BIT_TUNE_PC,
                        dsyslog ("mcli::ProvidesChannel: DEBUG DVB:%d Channel:%s * 'Priority >= 0 && Receiving (true)' is False", CardIndex () + 1, Channel->Name ());
			)
#endif
		}
	} else {
#ifdef DEBUG_TUNE_PC
		DEBUG_MASK(DEBUG_BIT_TUNE_PC,
                dsyslog ("mcli::ProvidesChannel: DEBUG DVB:%d Channel:%s * 'ProvidesTransponder(Channel)' is False", CardIndex () + 1, Channel->Name ());
		)
#endif
	}

#ifdef DEBUG_TUNE
	DEBUG_MASK(DEBUG_BIT_TUNE,
	dsyslog ("mcli::%s: DVB:%d Channel:%s, Prio:%d this->Prio:%d NeedsDetachReceivers:%d -> %d", __FUNCTION__, CardIndex () + 1, Channel->Name (), Priority, this->Priority (), needsDetachReceivers, result);
	)
#endif
	if (NeedsDetachReceivers) {
		*NeedsDetachReceivers = needsDetachReceivers;
	}
	return result;
}

void cMcliDevice::TranslateTypePos(int &type, int &pos, const int Source) const
{
#if VDRVERSNUM < 10713
	pos = Source;
	pos = ((pos & st_Neg) ? 1 : -1) * (pos & st_Pos);
#else
	int n = (Source & 0xffff);
	
	if (n > 0x00007FFF) {
		n |= 0xFFFF0000;
	}
	
	pos=abs(n);
	
#if VDRVERSNUM < 20100
	// Changed the sign of the satellite position value in cSource to reflect the standard
	// of western values being negative. The new member function cSource::Position() can be
	// used to retrieve the orbital position of a satellite.
	// see changelog vdr2.1.1 * found by horwath@dayside.net
	if (n > 0 ){
		pos = -pos;
	}
#endif

#endif
	if (pos) {
		pos += 1800;
	} else {
		pos = NO_SAT_POS;
	}
	
	type = Source & cSource::st_Mask;

        //printf("MCLI DEBUG type %d, cSource::stCable %d, cSource::stSat %d, cSource::stTerr %d \n", type, cSource::stCable, cSource::stSat, cSource::stTerr );
        //printf("MCLI DEBUG FE_QAM %d, FE_QPSK %d, FE_OFDM %d \n", FE_QAM, FE_QPSK, FE_OFDM);
       
	switch(type) {
                case cSource::stSat:
                        type = FE_QPSK;
                        break;
		case cSource::stCable: 
			type = FE_QAM;
			break;
		case cSource::stTerr:
			type = FE_OFDM;
			break;
		default:
			type = -1;
	}
}

bool cMcliDevice::SetChannelDevice (const cChannel * Channel, bool LiveView)
{
	bool is_scan = false;
	int pos;
	int type;
	bool s2;

	is_scan = !strlen(Channel->Name()) && !strlen(Channel->Provider());
	
#ifdef DEBUG_TUNE
	DEBUG_MASK(DEBUG_BIT_TUNE,
	dsyslog ("mcli::%s: Request tuning on DVB:%d to Channel:%s, Provider:%s, Source:%d, LiveView:%s, IsScan:%d CA:%d", __FUNCTION__, CardIndex () + 1, Channel->Name (), Channel->Provider (), Channel->Source (), LiveView ? "true" : "false", is_scan, Channel->Ca());
	)
#endif
	if (!m_enable) {
		return false;
	}

	bool triggerCam = not(m_debugmask & DEBUG_BIT_Action_SkipTriggerCam);
	if(m_cam_disable && (Channel->Ca() || triggerCam)) {
		LOGSKIP_MASK(LOGSKIP_BIT_SetChannelDevice_Reject,
		dsyslog ("mcli::%s: Reject tuning on DVB:%d to Channel:%s (%d), Provider:%s, Source:%d, LiveView:%s, IsScan:%d CA:%d (requires CAM, but disabled by option)", __FUNCTION__, CardIndex () + 1, Channel->Name (), Channel->Number(), Channel->Provider (), Channel->Source (), LiveView ? "true" : "false", is_scan, Channel->Ca());
		)
		return scrNotAvailable;
	};
	LOCK_THREAD;

	if(is_scan) {
		m_disabletimeout = TEMP_DISABLE_TIMEOUT_SCAN;
	} else if (GetCaOverride()) {
		m_disabletimeout = TEMP_DISABLE_TIMEOUT_CAOVERRIDE;
	} else {
		m_disabletimeout = TEMP_DISABLE_TIMEOUT_DEFAULT;
	}
	bool cam_force = m_mcli && m_mcli->CAMPresent() && LiveView;
	if(cam_force && !CheckCAM(Channel, true)) {
#ifdef DEBUG_TUNE
		DEBUG_MASK(DEBUG_BIT_TUNE,
		dsyslog("mcli::%s: No CAM on DVB %d available even after tried to steal one", __FUNCTION__, CardIndex () + 1);
		dsyslog("mcli::%s: CAMPresent: %d", __FUNCTION__, m_mcli->CAMPresent());
		)
#endif
		return false;
	}
	if(!GetCaOverride() && (Channel->Ca() || triggerCam) && !GetCaEnable()) {
		int slot = -1;
		if(Channel->Ca(0)<=0xff) {
			slot=Channel->Ca(0)&0x03;
			if(slot) {
				slot--;
			}
		}
		if(!(m_camref=m_mcli->CAMAlloc(NULL, slot))) {
#ifdef DEBUG_TUNE
			DEBUG_MASK(DEBUG_BIT_TUNE,
			dsyslog("mcli::%s: failed to get CAM on DVB %d (cam_force=%s)", __FUNCTION__, CardIndex () + 1, cam_force ? "true" : "false");
			)
#endif
			if(cam_force) {
				return false;
			}
		}
		if(m_camref) {
			SetCaEnable();
		}
	}
	TranslateTypePos(type, pos, Channel->Source());
#if VDRVERSNUM < 10702	
	s2=Channel->Modulation() == QPSK_S2 || Channel->Modulation() == PSK8;
#elif VDRVERSNUM < 10714	
	s2=Channel->System() == SYS_DVBS2;
#elif VDRVERSNUM < 10722
        cDvbTransponderParameters dtp(Channel->Parameters());
	s2=dtp.System() == SYS_DVBS2;
#elif VDRVERSNUM < 10723
	#error "vdr version not supported"
#else
	cDvbTransponderParameters dtp(Channel->Parameters());
	s2=dtp.System() == DVB_SYSTEM_2;
#endif	
	if(s2) {
		type = FE_DVBS2;
	}

	if(m_tunerref && (FE_QPSK == type) && (FE_DVBS2 == m_fetype)) type = FE_DVBS2;
	if(m_tunerref && (m_fetype != type || !m_mcli->TunerSatelitePositionLookup(m_tunerref, pos))) {
		m_mcli->TunerFree(m_tunerref);
		m_tunerref = NULL;
	}
	
	if(s2 && (m_fetype != FE_DVBS2)) {
		if(m_tunerref) {
			m_mcli->TunerFree(m_tunerref);
			m_tunerref = NULL;
		}
		type=FE_DVBS2;
	}

	if(m_tunerref == NULL) {
		m_tunerref = m_mcli->TunerAlloc((fe_type_t)type, pos);
		if(m_tunerref == NULL && type == FE_QPSK) {
			type = FE_DVBS2;
			m_tunerref = m_mcli->TunerAlloc((fe_type_t)type, pos);
		}
		m_tuned = false;
		if(m_tunerref == NULL) {
			return false;
		}
		m_fetype = type;
	}

	m_pos = pos;

	if (IsTunedToTransponder (Channel) && !is_scan) {
		m_chan = *Channel;

#ifdef DEBUG_TUNE
		DEBUG_MASK(DEBUG_BIT_TUNE,
                dsyslog("mcli::%s: Already tuned to transponder on DVB %d", __FUNCTION__, CardIndex () + 1);
		)
#endif
		return true;
	} else {
		memset (&m_ten, 0, sizeof (tra_t));
	}
	memset (&m_sec, 0, sizeof (recv_sec_t));
	memset (&m_fep, 0, sizeof (struct dvb_frontend_parameters));
	m_chan = *Channel;

#ifdef DEBUG_TUNE
	DEBUG_MASK(DEBUG_BIT_TUNE,
	dsyslog ("mcli::%s: Really tuning now DVB:%d to Channel:%s, Provider:%s, Source:%d, LiveView:%s, IsScan:%d CA:%d", __FUNCTION__, CardIndex () + 1, Channel->Name (), Channel->Provider (), Channel->Source (), LiveView ? "true" : "false", is_scan, Channel->Ca());
	)
#endif
	switch (m_fetype) {
	case FE_DVBS2:
	case FE_QPSK:{		// DVB-S

			int frequency = Channel->Frequency ();

#if VDRVERSNUM < 10714
			fe_sec_voltage_t volt = (Channel->Polarization () == 'v' || Channel->Polarization () == 'V' || Channel->Polarization () == 'r' || Channel->Polarization () == 'R') ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18;
#else
			fe_sec_voltage_t volt = (dtp.Polarization () == 'v' || dtp.Polarization () == 'V' || dtp.Polarization () == 'r' || dtp.Polarization () == 'R') ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18;
			frequency =::abs (frequency);   // Allow for C-band, where the frequency is less than the LOF
#endif		
			m_sec.voltage = volt;
			m_fep.frequency = frequency * 1000UL;
#if VDRVERSNUM < 10714			
			m_fep.inversion = fe_spectral_inversion_t (Channel->Inversion ());
#else
			m_fep.inversion = fe_spectral_inversion_t (dtp.Inversion ());
#endif
			m_fep.u.qpsk.symbol_rate = Channel->Srate () * 1000UL;
#if VDRVERSNUM < 10702				
//			m_fep.u.qpsk.fec_inner = fe_code_rate_t (Channel->CoderateH () | (Channel->Modulation () << 16));
			int modulation = Channel->Modulation ();
#if DVB_API_VERSION > 3
			if (modulation==PSK_8) // Needed if PSK_8 != PSK8
				modulation = PSK8;
			else if(modulation==PSK_8+4) // patched DVB_V5 QPSK for S2
				modulation = QPSK_S2;
#endif
			m_fep.u.qpsk.fec_inner = fe_code_rate_t (Channel->CoderateH () | (modulation << 16));
#elif VDRVERSNUM < 10714
			if(s2) {
				int modulation = 0;
				switch(Channel->Modulation ()) {
					case QPSK:
						modulation = QPSK_S2;
						break;
					case PSK_8:
						modulation = PSK8;
						break;
				}
				 m_fep.u.qpsk.fec_inner = fe_code_rate_t (Channel->CoderateH () | (modulation << 16));
			}
#else
			if(s2) {
				int modulation = 0;
				switch(dtp.Modulation ()) {
					case QPSK:
						modulation = QPSK_S2;
						break;
					case PSK_8:
						modulation = PSK8;
						break;
				}
				int coderateH = dtp.CoderateH ();
				switch(dtp.CoderateH()) {
				        case FEC_AUTO+1:                // DVB-API 5 FEC_3_5
						coderateH = FEC_AUTO+4; // MCLI-API  FEC_3_5
				                break;
					case FEC_AUTO+2:                // DVB-API 5 FEC_9_10
						coderateH = FEC_AUTO+5; // MCLI-API  FEC_9_10
						break;
				}
				m_fep.u.qpsk.fec_inner = fe_code_rate_t (coderateH | (modulation << 16));
			}
#endif			
		}
		break;
	case FE_QAM:{		// DVB-C

			// Frequency and symbol rate:
			m_fep.frequency = FrequencyToHz (Channel->Frequency ());
#if VDRVERSNUM < 10714			
			m_fep.inversion = fe_spectral_inversion_t (Channel->Inversion ());
			m_fep.u.qam.fec_inner = fe_code_rate_t (Channel->CoderateH ());
			m_fep.u.qam.modulation = fe_modulation_t (Channel->Modulation ());
#else
			m_fep.inversion = fe_spectral_inversion_t (dtp.Inversion ());
			m_fep.u.qam.fec_inner = fe_code_rate_t (dtp.CoderateH ());
			m_fep.u.qam.modulation = fe_modulation_t (dtp.Modulation ());
#endif
			m_fep.u.qam.symbol_rate = Channel->Srate () * 1000UL;
		}
		break;
	case FE_OFDM:{		// DVB-T

			// Frequency and OFDM paramaters:
			m_fep.frequency = FrequencyToHz (Channel->Frequency ());
#if VDRVERSNUM < 10714			
			m_fep.inversion = fe_spectral_inversion_t (Channel->Inversion ());
			m_fep.u.ofdm.bandwidth = fe_bandwidth_t (Channel->Bandwidth ());
			m_fep.u.ofdm.code_rate_HP = fe_code_rate_t (Channel->CoderateH ());
			m_fep.u.ofdm.code_rate_LP = fe_code_rate_t (Channel->CoderateL ());
			m_fep.u.ofdm.constellation = fe_modulation_t (Channel->Modulation ());
			m_fep.u.ofdm.transmission_mode = fe_transmit_mode_t (Channel->Transmission ());
			m_fep.u.ofdm.guard_interval = fe_guard_interval_t (Channel->Guard ());
			m_fep.u.ofdm.hierarchy_information = fe_hierarchy_t (Channel->Hierarchy ());
#else
			m_fep.inversion = fe_spectral_inversion_t (dtp.Inversion ());
			m_fep.u.ofdm.bandwidth = fe_bandwidth_t (dtp.Bandwidth ());
			m_fep.u.ofdm.code_rate_HP = fe_code_rate_t (dtp.CoderateH ());
			m_fep.u.ofdm.code_rate_LP = fe_code_rate_t (dtp.CoderateL ());
			m_fep.u.ofdm.constellation = fe_modulation_t (dtp.Modulation ());
			m_fep.u.ofdm.transmission_mode = fe_transmit_mode_t (dtp.Transmission ());
			m_fep.u.ofdm.guard_interval = fe_guard_interval_t (dtp.Guard ());
			m_fep.u.ofdm.hierarchy_information = fe_hierarchy_t (dtp.Hierarchy ());
#endif
		}
		break;
	default:
		esyslog ("mcli::%s: ERROR: attempt to set channel with unknown DVB frontend type", __FUNCTION__);
		return false;
	}

	recv_tune (m_r, (fe_type_t)m_fetype, m_pos, &m_sec, &m_fep, m_pids);
	m_tuned = true;
	if((m_pids[0].pid==-1)) {
		dvb_pid_t pi;
		memset(&pi, 0, sizeof(dvb_pid_t));
		recv_pid_add (m_r, &pi);
//		printf("add dummy pid 0 @ %p\n", this);
	}
#ifdef DEBUG_PIDS
	DEBUG_MASK(DEBUG_BIT_PIDS,
	dsyslog ("mcli::%s: %p Pidsnum:%d m_pidsnum:%d", __FUNCTION__, m_r, m_mcpidsnum, m_pidsnum);
	for (int i = 0; i < m_mcpidsnum; i++) {
		dsyslog ("mcli::%s: Pid:%d Id:%d", __FUNCTION__, m_pids[i].pid, m_pids[i].id);
	}
	)
#endif
	m_ten.lastseen=m_last=time(NULL);
	m_showtuning = true;
	return true;
}

bool cMcliDevice::HasLock (int TimeoutMs) const
{
	dbg ("HasLock TimeoutMs:%d\n", TimeoutMs);

	if ((m_ten.s.st & FE_HAS_LOCK) || !TimeoutMs) {
		return m_ten.s.st & FE_HAS_LOCK;
	}
	cMutexLock MutexLock ((cMutex*)&mutex); // ugly hack to lock a mutex in a const member
	if (TimeoutMs && !(m_ten.s.st & FE_HAS_LOCK)) {
		((cCondVar&)m_locked).TimedWait ((cMutex&)mutex, TimeoutMs); // ugly hack to lock a mutex in a const member
	}
	if (m_ten.s.st & FE_HAS_LOCK) {
		return true;
	}
	return false;
}

bool cMcliDevice::SetPid (cPidHandle * Handle, int Type, bool On)
{
#ifdef DEBUG_TUNE
	DEBUG_MASK(DEBUG_BIT_TUNE,
	dsyslog ("mcli::%s: DVB:%d Pid:%d (%s), Type:%d, On:%d, used:%d sid:%d ca_enable:%d channel_ca:%d", __FUNCTION__, CardIndex () + 1, Handle->pid, m_chan.Name(), Type, On, Handle->used, m_chan.Sid(), GetCaEnable(), m_chan.Ca (0));
	)
#endif
	dvb_pid_t pi;
	memset (&pi, 0, sizeof (dvb_pid_t));
	if (!m_enable) {
		return false;
	}
	// LOCK_THREAD; // deactivated because resulting in deadlock with osdteletext and zapping
	if (Handle->pid && (On || !Handle->used)) {
		m_pidsnum += On ? 1 : -1;
		if (m_pidsnum < 0) {
			m_pidsnum = 0;
		}

		if (On) {
			pi.pid = Handle->pid;
			if (GetCaEnable() && (m_chan.Ca(0) || (m_camref && m_camref->trigger && (!m_camref->triggerSid || m_camref->triggerSid == m_chan.Sid())))) {
				pi.id= m_chan.Sid();
				if(m_chan.Ca(0)<=0xff) {
					pi.priority=m_chan.Ca(0)&0x03;
				}
				if (!m_chan.Ca(0)) {
					m_camref->triggerSid = pi.id;
#ifdef DEBUG_TUNE
					DEBUG_MASK(DEBUG_BIT_TUNE,
					dsyslog("Mcli::%s: FTA CAM-Trigger Pid %d Sid %d", __FUNCTION__, pi.pid, pi.id);
					)
#endif
				}
			} 
#ifdef ENABLE_DEVICE_PRIORITY
			int Prio = Priority();
			if(Prio>50) // Recording prio high
				pi.priority |= 3<<2;
			else if(Prio > 10) // Recording prio normal
				pi.priority |= 2<<2;
			else if(Prio >= 0) // Recording prio low
				pi.priority |= 1<<2;
			else if(Prio == -1) // Live
				pi.priority |= 1<<2;
#endif
#ifdef DEBUG_PIDS_ADD_DEL
			DEBUG_MASK(DEBUG_BIT_PIDS_ADD_DEL,
			dsyslog ("mcli::%s: DVB:%d Add Pid:%d Sid:%d Type:%d Prio:%d %d", __FUNCTION__, CardIndex () + 1, pi.pid, pi.id, Type, pi.priority, m_chan.Ca(0));
			)
#endif
			recv_pid_add (m_r, &pi);
		} else {
#ifdef DEBUG_PIDS_ADD_DEL
			DEBUG_MASK(DEBUG_BIT_PIDS_ADD_DEL,
			dsyslog ("mcli::%s: DVB:%d Del Pid:%d", __FUNCTION__, CardIndex () + 1, Handle->pid);
			)
#endif
			if (m_camref)
				m_camref->trigger = false;
			recv_pid_del (m_r, Handle->pid);
		}
	}
	m_mcpidsnum = recv_pids_get (m_r, m_pids);
#ifdef DEBUG_PIDS
	DEBUG_MASK(DEBUG_BIT_PIDS,
	dsyslog ("mcli::%s: %p Pidsnum:%d m_pidsnum:%d m_filternum:%d", __FUNCTION__, m_r, m_mcpidsnum, m_pidsnum, m_filternum);
	for (int i = 0; i < m_mcpidsnum; i++) {
		dsyslog ("mcli::%s: Pid:%d Id:%d", __FUNCTION__, m_pids[i].pid, m_pids[i].id);
	}
	)
#endif
	m_last=time(NULL);
	return true;
}

bool cMcliDevice::OpenDvr (void)
{
	m_dvr_open = true;
	return true;
}

void cMcliDevice::CloseDvr (void)
{
	m_dvr_open = false;
}

#ifdef GET_TS_PACKETS
int cMcliDevice::GetTSPackets (uchar * Data, int count)
{
	if (!m_enable || !m_dvr_open) {
		return 0;
	}
	m_PB->GetEnd ();

	int size;
	uchar *buf = m_PB->GetStartMultiple (count, &size, 0, 0);
	if (buf) {
		memcpy (Data, buf, size);
		m_PB->GetEnd ();
		return size;
	} else {
		return 0;
	}
}				// cMcliDevice::GetTSPackets
#endif

bool cMcliDevice::GetTSPacket (uchar * &Data)
{
#ifdef USE_VDR_PACKET_BUFFER
	if (!m_enable || !m_dvr_open) return false;
	Data = NULL;
	int Count = 0;
	if(delivered) {
		m_PB->Del(TS_SIZE);
		delivered=false;
	}
	uchar *p = m_PB->Get(Count);
	if (p && Count >= TS_SIZE) {
		if (*p != TS_SYNC_BYTE) {
			for (int i = 1; i < Count; i++) {
				if (p[i] == TS_SYNC_BYTE) {
					Count = i;
					break;
				}
			}
			m_PB->Del(Count);
			esyslog("mcli::%s: skipped %d bytes to sync on TS packet on DVB %d", __FUNCTION__, Count, CardIndex());
			return true;
		}
		delivered = true;
		Data = p;
	}
	return true;
#else
	if (m_enable && m_dvr_open) {
		m_PB->GetEnd ();

		int size;
		Data = m_PB->GetStart (&size, 0, 0);
	}
	return true;
#endif
}

int cMcliDevice::OpenFilter (u_short Pid, u_char Tid, u_char Mask)
{
	if (!m_enable) {
		return -1;
	}
	LOCK_THREAD;
	m_filternum++;
//	printf ("OpenFilter (%d/%d/%d) pid:%d tid:%d mask:%04x %s\n", m_filternum, m_pidsnum, m_mcpidsnum, Pid, Tid, Mask, ((m_filternum+m_pidsnum) < m_mcpidsnum) ? "PROBLEM!!!":"");
	dvb_pid_t pi;
	memset (&pi, 0, sizeof (dvb_pid_t));
	pi.pid = Pid;
//      printf ("Add Pid:%d\n", pi.pid);
	recv_pid_add (m_r, &pi);
	m_mcpidsnum = recv_pids_get (m_r, m_pids);
#ifdef DEBUG_PIDS
	DEBUG_MASK(DEBUG_BIT_PIDS,
	dsyslog ("mcli::%s: %p Pidsnum:%d m_pidsnum:%d", __FUNCTION__, m_r, m_mcpidsnum, m_pidsnum);
	for (int i = 0; i < m_mcpidsnum; i++) {
		dsyslog ("mcli::%s: Pid:%d Id:%d", __FUNCTION__, m_pids[i].pid, m_pids[i].id);
	}
	)
#endif
	return m_filters->OpenFilter (Pid, Tid, Mask);
}

void cMcliDevice::CloseFilter (int Handle)
{
	if (!m_enable) {
		return;
	}

	LOCK_THREAD;
	int pid = m_filters->CloseFilter (Handle);
	
	if ( pid != -1) {
//		printf("CloseFilter FULL\n");
		recv_pid_del (m_r, pid);
		m_mcpidsnum = recv_pids_get (m_r, m_pids);
	}
	m_filternum--;
//	printf ("CloseFilter(%d/%d/%d) pid:%d %s\n", m_filternum, m_pidsnum, m_mcpidsnum, pid, pid==-1?"PID STILL USED":"");
}

#ifdef DEVICE_ATTRIBUTES
/* Attribute classes for dvbdevice
  main  main attributes
      .name (String) "DVB", "IPTV", ...

  fe : frontend attributes (-> get from tuner)
      .type (int) FE_QPSK, ...
      .name (string) Tuner name
      .status,.snr,... (int)
*/
int cMcliDevice::GetAttribute (const char *attr_name, uint64_t * val)
{
	int ret = 0;
	uint64_t rval = 0;

	if (!strcmp (attr_name, "fe.status")) {
		rval = m_ten.s.st;
		if ((m_ten.lastseen+LASTSEEN_TIMEOUT)>time(0)) {
			rval|= (m_ten.rotor_status&3)<<8;
			rval|= (1+m_ten.slot)<<12;
		}		                                                                
	} else if (!strcmp (attr_name, "fe.signal")) {
		rval = m_ten.s.strength;
	} else if (!strcmp (attr_name, "fe.snr")) {
		rval = m_ten.s.snr;
	} else if (!strcmp (attr_name, "fe.ber")) {
		rval = m_ten.s.ber;
	} else if (!strcmp (attr_name, "fe.unc")) {
		rval = m_ten.s.ucblocks;
	} else if (!strcmp (attr_name, "fe.type")) {
		rval = m_fetype;
	} else if (!strcmp (attr_name, "is.mcli")) {
		rval = 1;
	} else if (!strcmp (attr_name, "fe.lastseen")) {
		rval = m_ten.lastseen;
	} else
		ret = -1;

	if (val)
		*val = rval;
	return ret;
}

int cMcliDevice::GetAttribute (const char *attr_name, char *val, int maxret)
{
	int ret = 0;
	if (!strcmp (attr_name, "fe.uuid")) {
		strncpy (val, "NetCeiver", maxret);
		val[maxret - 1] = 0;
	} else if (!strcmp (attr_name, "fe.name")) {
		strncpy (val, "NetCeiver", maxret);
		val[maxret - 1] = 0;
	} else if (!strncmp (attr_name, "main.", 5)) {
		if (!strncmp (attr_name + 5, "name", 4)) {
			if (val && maxret > 0) {
				strncpy (val, "NetCeiver", maxret);
				val[maxret - 1] = 0;
			}
			return 0;
		}
	} else {
		ret = -1;
	}
	return ret;
}

#endif

int cMcliDevice::SignalStrength(void) const 
{
       return int(m_ten.s.strength/65536.*100.);
} 

int cMcliDevice::SignalQuality(void) const 
{
       return int(m_ten.s.snr/65536.*100.);
}

#if VDRVERSNUM >= 10713
const cChannel *cMcliDevice::GetCurrentlyTunedTransponder(void) const
{
	if (!m_enable) {
#ifdef DEBUG_RESOURCES
		DEBUG_MASK(DEBUG_BIT_RESOURCES,
		dsyslog("mcli::%s: DVB:%d m_fetype=%d not enabled", __FUNCTION__, CardIndex () + 1, m_fetype);
		)
#endif
		return NULL;
	};
	if (!m_tuned) {
#ifdef DEBUG_RESOURCES
		DEBUG_MASK(DEBUG_BIT_RESOURCES,
		dsyslog("mcli::%s: DVB:%d m_fetype=%d not tuned", __FUNCTION__, CardIndex () + 1, m_fetype);
		)
#endif
		return NULL;
	};
#ifdef DEBUG_RESOURCES
	DEBUG_MASK(DEBUG_BIT_RESOURCES,
	dsyslog("mcli::%s: DVB:%d m_chan.Name='%s', m_chan.Number=%d m_fetype=%d", __FUNCTION__, CardIndex () + 1, m_chan.Name(), m_chan.Number(), m_fetype);
	)
#endif
	return &m_chan;
}
#endif

#if VDRVERSNUM >= 10728
cString cMcliDevice::DeviceType(void) const
{
	if (!m_enable) {
#if 0
#ifdef DEBUG_RESOURCES
		DEBUG_MASK(DEBUG_BIT_RESOURCES,
		dsyslog("mcli::%s: m_fetype=%d not enabled", __FUNCTION__, m_fetype);
		)
#endif
#endif
		return "";
	};
	switch (m_fetype) {
		case FE_QAM:
			return "DVB-C";
		case FE_OFDM:
			return "DVB-T";
		case FE_QPSK:
			return "DVB-S";
		case FE_DVBS2:
			return "DVB-S2";
		default:
			return "";
	};
}
#endif
