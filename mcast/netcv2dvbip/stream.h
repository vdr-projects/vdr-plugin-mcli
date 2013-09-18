#ifndef	__STREAM_H
#define __STREAM_H

#ifdef WIN32
#include <winsock2.h>
#endif

#include "clist.h"
#include "thread.h"
#include "misc.h"

class cStream : public cListObject, public cThread
{
	public:
			cStream(int channum, in_addr_t addr, int portnum);
			~cStream(void);
			bool StartStream(in_addr_t bindaddr);
			void StopStream();

	private:
			void *handle;
			SCKT udp_socket;
			struct sockaddr_in peer;
			int channum;
			in_addr_t addr;
			int m_portnum;
			
			virtual void Action();
};

#endif
