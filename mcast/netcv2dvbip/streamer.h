#ifndef	__STREAMER_H
#define __STREAMER_H

#include "misc.h"

class cMulticastGroup;
class cIgmpMain;

class cStreamer
{
	public:
			cStreamer();
			
			void Run();
			void Stop();
			void SetBindIf(iface_t bindif);
			void SetStreamPort(int portnum);
			void SetTable(int table);
			void SetNumGroups(int numgroups);

			bool IsGroupinRange(in_addr_t groupaddr);
			void StartMulticast(cMulticastGroup* Group);
			void StopMulticast(cMulticastGroup* Group);
			

	private:
			cIgmpMain* m_IgmpMain;
			in_addr_t m_bindaddr;
			iface_t m_bindif;
			int m_table;
			int m_portnum;
			int m_numgroups;
};

#endif
