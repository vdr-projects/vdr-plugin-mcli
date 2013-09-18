/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#ifndef VDR_STREAMDEV_FILTER_H
#define VDR_STREAMDEV_FILTER_H

#include <vdr/config.h>
#include <vdr/tools.h>
#include <vdr/thread.h>
#include "packetbuffer.h"

class cMcliFilter;
class cMcliPid;

class cMcliPidList:public cList < cMcliPid >
{
      public:
	cMcliPidList (void)
	{
	};
	~cMcliPidList () {
	};
	int GetTidFromPid (int pid);
	void SetPid (int Pid, int Tid);
};

class cMcliFilters:public cList < cMcliFilter >, public cThread
{
      private:
	cMyPacketBuffer * m_PB;
	cMcliPidList m_pl;
	bool m_closed;

      protected:
	  virtual void Action (void);
	void GarbageCollect (void);
  void ProcessChunk(u_short pid, const uchar *block, int len, bool Pusi);

      public:
	  cMcliFilters (void);
	  virtual ~ cMcliFilters ();
	bool WantPid (int pid);
	int GetTidFromPid (int pid);
	int GetPid (int Handle);
	cMcliFilter *GetFilter (int Handle);
	int PutTS (const uchar * data, int len);
	int OpenFilter (u_short Pid, u_char Tid, u_char Mask);
	int CloseFilter (int Handle);
};

#endif // VDR_STREAMDEV_FILTER_H
