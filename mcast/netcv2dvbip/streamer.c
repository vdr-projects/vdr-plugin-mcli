#include "defs.h"

#include <stdio.h>

#include "misc.h"
#include "igmp.h"
#include "streamer.h"
#include "stream.h"

#define MULTICAST_PRIV_MIN (0xEFFF0000)
#define MULTICAST_PRIV_MAX (0xEFFF1000)

cStreamer::cStreamer()
{
	m_IgmpMain = NULL;
	m_bindaddr = 0;
	m_portnum = 0;
	m_table = 0;
}

void cStreamer::SetBindIf(iface_t bindif)
{
	m_bindaddr = bindif.ipaddr;
	m_bindif = bindif;
}

void cStreamer::SetStreamPort(int portnum)
{
        m_portnum = portnum;
}

void cStreamer::SetTable(int table)
{
        m_table = table;
}

void cStreamer::Run()
{
	if ( m_IgmpMain == NULL )
	{
		m_IgmpMain = new cIgmpMain(this, m_bindif, m_table);
		m_IgmpMain->StartListener();
	}
	return;
}

void cStreamer::Stop()
{
	if ( m_IgmpMain )
	{
		m_IgmpMain->Destruct();
		delete m_IgmpMain;
		m_IgmpMain = NULL;
	}
	return;
}

void cStreamer::SetNumGroups(int numgroups)
{
	m_numgroups = numgroups;
}

bool cStreamer::IsGroupinRange(in_addr_t groupaddr)
{
        in_addr_t g = htonl(groupaddr);
        if ( (g > MULTICAST_PRIV_MIN) && (g <= MULTICAST_PRIV_MIN+m_numgroups) 
		&&(g <= MULTICAST_PRIV_MAX) )
        {
                return true;
        }
        return false;
}

void cStreamer::StartMulticast(cMulticastGroup* Group)
{
        in_addr group;
        group.s_addr = Group->group;
        unsigned long channel = htonl(Group->group) - MULTICAST_PRIV_MIN;

        printf("START Channel %d on Multicast Group: %s\n", 
		(unsigned short) channel, inet_ntoa(group));
        if (Group->stream == NULL)
        {
                Group->stream = new cStream(channel,  Group->group, m_portnum);
                Group->stream->StartStream(m_bindaddr);
        }
}

void cStreamer::StopMulticast(cMulticastGroup* Group)
{
        in_addr group;
        group.s_addr = Group->group;

        printf("STOP Multicast Group: %s\n", inet_ntoa(group));
        if (Group->stream)
        {
                Group->stream->StopStream();
                delete Group->stream;
                Group->stream = NULL;
        }
}
