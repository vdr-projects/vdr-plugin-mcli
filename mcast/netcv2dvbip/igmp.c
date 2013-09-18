#include "defs.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#ifndef __MINGW32__
#include <mstcpip.h>
#else
#define SIO_RCVALL_IGMPMCAST  _WSAIOW(IOC_VENDOR,3)
#endif
#else
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#ifdef APPLE
#include <netinet/ip_mroute.h>
#else
#define _LINUX_IN_H
#include <linux/mroute.h>
#endif
#endif

#include <stdio.h>
#include <errno.h>

#include "igmp.h"
#include "streamer.h"
#include "misc.h"

#ifndef IGMP_ALL_HOSTS
#define IGMP_ALL_HOSTS htonl(0xE0000001L)
#endif
#ifndef IGMP_ALL_ROUTER
#define IGMP_ALL_ROUTER htonl(0xE0000002L)
#endif
#ifndef IGMP_ALL_V3REPORTS
#define IGMP_ALL_V3REPORTS htonl(0xE0000016L)
#endif

// IGMP parameters according to RFC2236. All time values in seconds.
#define IGMP_ROBUSTNESS 2
#define IGMP_QUERY_INTERVAL 125
#define IGMP_QUERY_RESPONSE_INTERVAL 10
#define IGMP_GROUP_MEMBERSHIP_INTERVAL \
	(2 * IGMP_QUERY_INTERVAL + IGMP_QUERY_RESPONSE_INTERVAL)
#define IGMP_OTHER_QUERIER_PRESENT_INTERVAL \
	(2 * IGMP_QUERY_INTERVAL + IGMP_QUERY_RESPONSE_INTERVAL / 2)
#define IGMP_STARTUP_QUERY_INTERVAL (IGMP_QUERY_INTERVAL / 4)
#define IGMP_STARTUP_QUERY_COUNT IGMP_ROBUSTNESS
// This value is 1/10 sec. RFC default is 10. Reduced to minimum to
// free unused channels ASAP
#define IGMP_LAST_MEMBER_QUERY_INTERVAL_TS 1
#define IGMP_LAST_MEMBER_QUERY_COUNT IGMP_ROBUSTNESS


// operations on struct timeval
#define TV_CMP(a, cmp, b) \
	(a.tv_sec == b.tv_sec ? a.tv_usec cmp b.tv_usec : a.tv_sec cmp b.tv_sec)
#define TV_SET(tv) (tv.tv_sec || tv.tv_usec)
#define TV_CLR(tv) memset(&tv, 0, sizeof(tv))
#define TV_CPY(dst, src) memcpy(&dst, &src, sizeof(dst))
#define TV_ADD(dst, ts) dst.tv_sec += ts / 10; \
	dst.tv_usec += (ts % 10) * 100000; \
	if (dst.tv_usec >= 1000000) { dst.tv_usec -= 1000000; dst.tv_sec++; }

/* Taken from http://tools.ietf.org/html/rfc1071 */
uint16_t inetChecksum(uint16_t *addr, int count)
{
        uint32_t sum = 0;
        while (count > 1) {
                sum += *addr++;
                count -= 2;
        }

        if( count > 0 )
                sum += * (uint8_t *) addr;

        while (sum>>16)
                sum = (sum & 0xffff) + (sum >> 16);

        return ~sum;
}


cMulticastGroup::cMulticastGroup(in_addr_t Group):
		group(Group),
		reporter(0),
		stream(NULL)
{
	TV_CLR(timeout);
	TV_CLR(v1timer);
	TV_CLR(retransmit);
}

cIgmpListener::cIgmpListener(cIgmpMain* igmpmain)
	: cThread("IGMP listener")
{
	m_IgmpMain = igmpmain;
}

bool cIgmpListener::Membership(in_addr_t mcaddr, bool Add)
{
	int rc = 0;

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = mcaddr;
    mreq.imr_interface.s_addr = m_bindaddr;
    
	rc = ::setsockopt(m_socket, IPPROTO_IP, 
		Add ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP,
		(const char*) &mreq, sizeof(mreq));
    if (rc < 0)
    {
	printf("IGMP: unable to %s %s", Add ? "join" : "leave", \
		inet_ntoa(mreq.imr_multiaddr));
#ifndef WIN32
        if (errno == ENOBUFS)
		printf("consider increasing sys.net.ipv4.igmp_max_memberships");
	else
#endif
	log_socket_error("IGMP setsockopt(IP_ADD/DROP_MEMBERSHIP)");
	return false;
    }

	return true;
}

void cIgmpListener::Destruct(void)
{
	int rc = 0;
	in_addr_t defaultaddr = INADDR_ANY;

	Cancel(3);
	
	Membership(IGMP_ALL_HOSTS, false);
	Membership(IGMP_ALL_ROUTER, false);
	Membership(IGMP_ALL_V3REPORTS, false);

	rc = setsockopt(m_socket, IPPROTO_IP, IP_MULTICAST_IF,
		(char *)&defaultaddr, sizeof(defaultaddr));
	if ( rc < 0 )
	{
		log_socket_error("IGMP setsockopt(IP_MULTICAST_IF)");
	}

#ifdef WIN32
	closesocket(m_socket);
#else
	rc = ::setsockopt( m_socket, IPPROTO_IP, MRT_DONE, NULL, 0); 
	if ( rc < 0 )
	{
		log_socket_error("IGMP setsockopt(MRT_DONE)");
	}
	close(m_socket);
#endif
}

bool cIgmpListener::Initialize(iface_t bindif, int table)
{
	int rc = 0;
#ifndef WIN32
	int val = 0;
#endif

	m_bindaddr = bindif.ipaddr;

	m_socket = ::socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);

	if (m_socket < 0)
	{
		log_socket_error("IGMP socket()");
		return false;
	}

#ifdef WIN32
	//-------------------------
	// Set the socket I/O mode: In this case FIONBIO
	// enables or disables the blocking mode for the
	// socket based on the numerical value of iMode.
	// If iMode = 0, blocking is enabled;
	// If iMode != 0, non-blocking mode is enabled.
	u_long iMode = 1;
	ioctlsocket(m_socket, FIONBIO, &iMode);
#else
	int x=fcntl(m_socket,F_GETFL,0);          // Get socket flags
	fcntl(m_socket,F_SETFL,x | O_NONBLOCK);   // Add non-blocking flag
#endif


#ifndef WIN32
#ifdef MRT_TABLE
	if ( table )
	{
		rc = ::setsockopt( m_socket, IPPROTO_IP, MRT_TABLE, (void*)&table, sizeof(table) ); 
		if ( rc < 0 )
		{
			log_socket_error("IGMP setsockopt(MRT_TABLE)");
			printf("IGMPv2 policy routing won't work! Please "
				"enable multicast policy routing option in"
				" the kernel configuration!\n");
		}
	}
#endif
	val = 1;
	rc = ::setsockopt( m_socket, IPPROTO_IP, MRT_INIT, 
		(void*)&val, sizeof(val) ); 
	if ( rc < 0 )
	{
		log_socket_error("IGMP setsockopt(MRT_INIT)");
		printf("IGMPv2 won't work! Please enable multicast router "
			"option in the kernel configuration!\n");
	}

	struct vifctl VifCtl;
	VifCtl.vifc_vifi  = 0;
	VifCtl.vifc_flags = 0;     // no tunnel, no source routing, register ?
	VifCtl.vifc_threshold  = 1;// Packet TTL must be at least 1 to pass them
	VifCtl.vifc_rate_limit = 0;// Ratelimit

	VifCtl.vifc_lcl_addr.s_addr = m_bindaddr;
	VifCtl.vifc_rmt_addr.s_addr = INADDR_ANY;

	rc = ::setsockopt( m_socket, IPPROTO_IP, MRT_ADD_VIF, 
		(char *)&VifCtl, sizeof( VifCtl ) ); 
	if ( rc < 0 )
	{
		log_socket_error("IGMP setsockopt(MRT_ADD_VIF)");
	}


#endif
/*
	rc = setsockopt(m_socket, IPPROTO_IP, IP_MULTICAST_IF,
		(char *)&m_bindaddr, sizeof(m_bindaddr));
	if ( rc < 0 )
	{
		log_socket_error("IGMP setsockopt(IP_MULTICAST_IF)");
		return false;
	}
*/
	/*
	val = 1;
	rc = ::setsockopt( m_socket, SOL_SOCKET, SO_REUSEADDR, 
		(const char*)&val, sizeof(val) ); 
	if ( rc < 0 )
	{
		log_socket_error("IGMP setsockopt(SO_REUSEADDR)");
		return false;
	}
	*/

	/*
	val = 1;
	rc = ::setsockopt( m_socket, SOL_IP, IP_ROUTER_ALERT, &val, 
		sizeof(val) );
	if ( rc < 0 )	
	{
		log_socket_error("IGMP setsockopt(IP_ROUTER_ALERT)");
		return false;
	}
	*/    

#ifdef WIN32
	//----------------------
	// Bind the socket. 
	// WIN32 requires this on a specific interface! 
	// INADDR_ANY is not allowed according to MSDN Lib for raw IGMP sockets.
	sockaddr_in m_LocalAddr;

	m_LocalAddr.sin_family = AF_INET;
	m_LocalAddr.sin_port   = 0;
	m_LocalAddr.sin_addr.s_addr = m_bindaddr;

	rc = ::bind( m_socket, (struct sockaddr*)&m_LocalAddr, 
		sizeof(m_LocalAddr) );
	if ( rc < 0 )
	{
		log_socket_error("IGMP bind()");
		return false;
	}

	int len = sizeof(struct sockaddr_in);
	rc = ::getsockname( m_socket, (struct sockaddr*)&m_LocalAddr, 
		(socklen_t*) &len );
	if ( rc < 0 )
	{
		log_socket_error("IGMP getsockname()");
		return false;
	}
#else
#ifndef APPLE
	// Normal bind() for this socket does not really work. the socket
	// does not receive anything then.
	// However, SO_BINDTODEVICE _does_ work. Maybe because of SOCK_RAW?!
	rc = setsockopt(m_socket, SOL_SOCKET, SO_BINDTODEVICE,
		(char *)&bindif.name, sizeof(bindif.name));
	if ( rc < 0 )
	{
		log_socket_error("IGMP setsockopt(SO_BINDTODEVICE)");
		return false;
	}
#endif
#endif

#ifdef WIN32
	DWORD dwCtlCode = SIO_RCVALL_IGMPMCAST;
	DWORD dwBytesSent = 0;
	rc = WSAIoctl(m_socket, dwCtlCode, &dwCtlCode, sizeof(dwCtlCode), NULL, 
		0, &dwBytesSent, NULL, NULL);
	if ( rc == SOCKET_ERROR )
	{
		log_socket_error("IGMP WSAIoctl()");
		return false;
	}
#endif


	if ( Membership(IGMP_ALL_HOSTS, true)
		&& Membership(IGMP_ALL_V3REPORTS, true) 
		&& Membership(IGMP_ALL_ROUTER, true))
	{
          Start();
          return true;
	}

	return false;
}

void cIgmpListener::Action()
{
	int recvlen;
	struct pollfd p;
	char recv_buf[8192];
	        

	p.fd = m_socket;
	p.events = POLLIN;

	while (Running())
	{
		switch (poll(&p,1,1000)) {
		case -1:
#ifndef WIN32
			if ( (errno != EINTR) && (errno != EWOULDBLOCK) )
#else
			int Rt;
			Rt = WSAGetLastError();
			if ( (Rt != WSAEINTR) && (Rt != WSAEWOULDBLOCK) )
#endif
				log_socket_error( "IGMP poll()" );
		case 0:
			continue;

		default:
			if (!(p.revents&POLLIN))
				continue;

			recvlen = recv(m_socket, (char*)&recv_buf, 
				sizeof(recv_buf), 0);
			if (recvlen < 0)
			{
#ifndef WIN32
                                if ( (errno == EINTR) || (errno == EWOULDBLOCK) )
#else
				int Rt;
                                Rt = WSAGetLastError();
                                if ( (Rt == WSAEINTR) || (Rt == WSAEWOULDBLOCK) )
#endif
                                        continue;
				log_socket_error("IGMP recv()");
				goto out;
			}
			Parse(recv_buf, recvlen);
			break;
		}
	}
out:;
}

void cIgmpListener::Parse(char* buffer, int len)
{
	char* buf = buffer;
	in_addr_t groupaddr;
	in_addr_t senderaddr;
	
	// Make sure we can at least extract the IP version
	if (len < (int) sizeof(char))
	{
	        return;
    	}
	// Check that IP version is IPV4 and that payload is IGMP
	if ( (HI_BYTE(*buf) == 4) && ( *(buf+9) == IPPROTO_IGMP) )
	{
		if (!quiet) {
			printf("IGMP: SrcAddr: %d.%d.%d.%d -> ", 
	   			(unsigned char) *(buf+12), 
				(unsigned char) *(buf+13), 
				(unsigned char) *(buf+14), 
				(unsigned char) *(buf+15) );
			printf("DstAddr: %d.%d.%d.%d\n", 
	   			(unsigned char) *(buf+16), 
				(unsigned char) *(buf+17), 
				(unsigned char) *(buf+18), 
				(unsigned char) *(buf+19) );
		}
		memcpy(&senderaddr, buf+12, 4);

	       	// skip rest of ip header and move to next to 
	       	// next protocol header
		len -= LO_BYTE(*buf) * 4;
		buf += LO_BYTE(*buf) * 4;

	        uint16_t chksum = ((*(buf+3)<<8)&0xFF00 ) | (*(buf+2) & 0x00FF);
	        *(buf+2) = 0;
	        *(buf+3) = 0;
	        if (chksum != inetChecksum((uint16_t *)buf, len))
	        {
			printf("IGMP: INVALID CHECKSUM 0x%04x 0x%04x - "
				"discarding packet.\n", chksum, 
				inetChecksum((uint16_t *)buf, len));
	                return;
	        }
		
		if ( (len == 8) )
		{   
			if ( *buf == 0x11 )
			{
				if (!quiet) {
					printf("IGMP: Version: %s, Type: "
						"Membership Query\n", 
						(*(buf+1) == 0 ) ? "1":"2");
					printf("        Group: %d.%d.%d.%d\n", 
						(unsigned char) *(buf+4), 
						(unsigned char) *(buf+5),
						(unsigned char) *(buf+6), 
						(unsigned char) *(buf+7));
				}
				memcpy(&groupaddr, buf+4, 4);
				m_IgmpMain->ProcessIgmpQueryMessage( groupaddr, 
					senderaddr );
			} 
			else if ( *buf == 0x12 )
			{
				if (!quiet)
					printf("IGMP: Version: 1, Type: "
						"Membership Report\n");
			} 
			else if (*buf == 0x16)
			{
				if (!quiet)
					printf("IGMP: Version: 2, Type: "
						"Membership Report\n");
			}
     		else if (*buf == 0x17)
			{
				if (!quiet)
					printf("IGMP: Version: 2, Type: "
						"Leave Group\n");
			}

			if( !quiet)
				printf("        Group: %d.%d.%d.%d\n", 
					(unsigned char) *(buf+4), 
					(unsigned char) *(buf+5), 
					(unsigned char) *(buf+6), 
					(unsigned char) *(buf+7));
			memcpy(&groupaddr, buf+4, 4);
			m_IgmpMain->ProcessIgmpReportMessage( (int) *buf, 
				groupaddr, senderaddr);
		}
		else if ( (*buf == 0x11) && (len > 8) )
		{
			if (!quiet) {
				printf("IGMP: Version: 3, Type: "
					"Membership Query, Maximum "
					"Response Time: %d\n",*(buf+1));
				printf("        Group: %d.%d.%d.%d\n", 
					(unsigned char) *(buf+4), 
					(unsigned char) *(buf+5), 
					(unsigned char) *(buf+6), 
					(unsigned char) *(buf+7));
			}
			memcpy(&groupaddr, buf+4, 4);
			m_IgmpMain->ProcessIgmpQueryMessage( groupaddr, 
				senderaddr );

		}
		else if ( (*buf == 0x22) && (len > 8) )
		{
			unsigned short numrecords = ntohs((unsigned short)*
				(buf+7)<<8|*(buf+6));
			if (!quiet)
				printf("IGMP: Version: 3, Type: Membership "
					"Report,  Number of Records: %d\n",
					numrecords);

			// Skip Header and move to records
			buf += 8;
			for(int i = 0; i<numrecords; i++)
			{
				unsigned short numsources = (unsigned short)*
					(buf+3)<<8|*(buf+2);
				if (!quiet) {
					printf("   ---> Record No: %d, Type: "
						"%d, Number of Sources: %d\n", 
						i+1, *buf, numsources);
					printf("        Group: %d.%d.%d.%d\n", 
						(unsigned char) *(buf+4), 
						(unsigned char) *(buf+5), 
						(unsigned char) *(buf+6), 
						(unsigned char) *(buf+7));
				}
				memcpy(&groupaddr, buf+4, 4);
				m_IgmpMain->ProcessIgmpReportMessage( 
					(int) *buf, groupaddr, senderaddr);

				if (!quiet) for(int j = 0; j<numsources; j++)
				{
					printf("           Sources: "
						"%d.%d.%d.%d\n", 
						(unsigned char) *(buf+j*4), 
						(unsigned char) *(buf+j*4+1), 
						(unsigned char) *(buf+j*4+2), 
						(unsigned char) *(buf+j*4+3));
					// move to next record: header bytes +
					// aux data len 
				}
				buf += 8 + *(buf+1) + (numsources *4);
			}
		}
		else
		{
			printf("IGMP: Invalid packet\n");
		}
	}

}

void cIgmpListener::IGMPSendQuery(in_addr_t Group, int Timeout)
{
        struct sockaddr_in dst;
        uint16_t query[6];
        
        dst.sin_family = AF_INET;
        dst.sin_port = IPPROTO_IGMP;
        dst.sin_addr.s_addr = Group;
        
        query[0] = ((Timeout * 10)&0xFF)<<8 | 0x11 ; // Membership Query
        query[1] = 0x0000; // Checksum
        if ( Group == IGMP_ALL_HOSTS )
        {   // General Query
            query[2] = 0x0000;
            query[3] = 0x0000;
        }
        else 
        {   // Group Query
            memcpy(&(query[2]), &Group, 4);
        }
        query[4] = IGMP_QUERY_INTERVAL<<8 | 0x00 ; // S=0, QRV=0
        query[5] = 0x0000; // 0 sources
	uint16_t chksum = inetChecksum( (uint16_t *) &query, sizeof(query));
        query[1] = chksum;

        for (int i = 0; i < 5 && ::sendto(m_socket, (const char*) &query, 
		sizeof(query), 0, (sockaddr*)&dst, sizeof(dst)) < 0 ; i++) 
	{
#ifndef WIN32
		if (errno != EAGAIN && errno != EWOULDBLOCK) 
#else
		if (WSAGetLastError() != WSAEWOULDBLOCK) 
#endif
		{
			printf("IGMP: unable to send query for group %s:", 
				inet_ntoa(dst.sin_addr));
                        log_socket_error("IGMP sendto()");
                        break;
                }

                cCondWait::SleepMs(10);
        }
}


// -------------------------------------------------------------------------------------------------------------------------

cIgmpMain::cIgmpMain(cStreamer* streamer, iface_t bindif, int table)
	: cThread("IGMP timeout handler")
{
	m_bindaddr = bindif.ipaddr;
	m_bindif = bindif;
	m_table = table;

	m_IgmpListener = new cIgmpListener(this);
	m_StartupQueryCount = IGMP_STARTUP_QUERY_COUNT;
	m_Querier = true;
	m_streamer = streamer;
	m_stopping = 0;
	TV_CLR(m_GeneralQueryTimer);
}

cIgmpMain::~cIgmpMain(void)
{
}

void cIgmpMain::Destruct(void)
{
	if (m_IgmpListener)
	{
		m_IgmpListener->Destruct();
	}
	
	// Wake up timeout thread in case it is currently sleeping
	m_stopping=1;
	m_CondWait.Signal();

	// Now try to stop thread. After 3 seconds it will be canceled.
	Cancel(3);
	
	cMulticastGroup *del = NULL;
	
	for (cMulticastGroup *group = m_Groups.First(); group; 
		group = m_Groups.Next(group))
	{
		if ( group->stream ) 
		{
			m_streamer->StopMulticast(group);
		}
		if (del)
			m_Groups.Del(del);
		del = group;
	}
	if (del)
		m_Groups.Del(del);

}

bool cIgmpMain::StartListener(void)
{
	if ( m_IgmpListener && m_IgmpListener->Initialize(m_bindif, m_table) )
	{
		Start();
		return true;
	}
	else
	{
		printf("cIgmpMain: Cannot create IGMP listener.\n");
		return false;
	}
}

void cIgmpMain::ProcessIgmpQueryMessage(in_addr_t groupaddr, 
	in_addr_t senderaddr)
{ 
	if (ntohl(senderaddr) < ntohl(m_bindaddr))
	{
		in_addr sender;
		sender.s_addr = senderaddr;
		printf("IGMP: Another Querier with lower IP address (%s) "
			"is active.\n", inet_ntoa(sender));
		IGMPStartOtherQuerierPresentTimer();
	}
}

void cIgmpMain::ProcessIgmpReportMessage(int type, in_addr_t groupaddr, 
	in_addr_t senderaddr)
{
	cMulticastGroup* group;

	if ( !m_streamer->IsGroupinRange(groupaddr) )
		return;

	LOCK_THREAD;

	switch (type)
	{
	case IGMPV3_MR_MODE_IS_INCLUDE:
		 break;

	case IGMPV1_MEMBERSHIP_REPORT:
	case IGMPV2_MEMBERSHIP_REPORT:
	case IGMPV3_MR_MODE_IS_EXCLUDE:
	case IGMPV3_MR_CHANGE_TO_EXCLUDE:
		group = FindGroup(groupaddr);
		if (!group) 
		{
			group = new cMulticastGroup(groupaddr);
			m_Groups.Add(group);
		}
		if (!group->stream)
		{
			m_streamer->StartMulticast(group);
		}
		IGMPStartTimer(group, senderaddr);
		if (type == IGMPV1_MEMBERSHIP_REPORT)
			IGMPStartV1HostTimer(group);
		break;

	case IGMPV2_LEAVE_GROUP:
	case IGMPV3_MR_CHANGE_TO_INCLUDE:
		group = FindGroup(groupaddr);
		if (group && !TV_SET(group->v1timer)) 
		{
			if (group->reporter == senderaddr) 
			{
				IGMPStartTimerAfterLeave(group, m_Querier ? 
					IGMP_LAST_MEMBER_QUERY_INTERVAL_TS : 1);
					//Igmp->igmp_code);
				if (m_Querier)
					IGMPSendGroupQuery(group);
				IGMPStartRetransmitTimer(group);
			}
			m_CondWait.Signal();
		}
		break;

	case IGMPV3_MR_ALLOW_NEW_SOURCES:
		printf("IGMPv3 Group Record Type \"ALLOW_NEW_SOURCES\" "
			"ignored.\n");
		break;
	case IGMPV3_MR_BLOCK_OLD_SOURCES:
		printf("IGMPv3 Group Record Type \"BLOCK_OLD_SOURCES\" "
			"ignored.\n");
		break;
	default:
		printf("Unknown IGMPv3 Group Record Type.\n");
	}
}

void cIgmpMain::Action()
{
	while (Running())
	{	
		struct timeval now;
		struct timeval next;

		gettimeofday(&now, NULL);
		TV_CPY(next, now);
		next.tv_sec += IGMP_QUERY_INTERVAL;

		cMulticastGroup *del = NULL;

		{// <-- ThreadLock is locked only within this block!
			LOCK_THREAD;

			if (TV_CMP(m_GeneralQueryTimer, <, now)) {
				if (!quiet)
				    printf("IGMP: Starting General Query.\n");
				IGMPSendGeneralQuery();
				IGMPStartGeneralQueryTimer();
			}
			if (TV_CMP(next, >, m_GeneralQueryTimer))
				TV_CPY(next, m_GeneralQueryTimer);

			for (cMulticastGroup *group = m_Groups.First(); group; 
				group = m_Groups.Next(group)) 
			{
				if (TV_CMP(group->timeout, <, now)) 
				{
					m_streamer->StopMulticast(group);
					IGMPClearRetransmitTimer(group);
					if (del)
						m_Groups.Del(del);
					del = group;
				}
				else if (m_Querier && TV_SET(group->retransmit) 
					&& TV_CMP(group->retransmit, <, now)) {
						IGMPSendGroupQuery(group);
						IGMPStartRetransmitTimer(group);
						if (TV_CMP(next, >, 
						    group->retransmit))
							TV_CPY(next, 
							    group->retransmit);
				}
				else if (TV_SET(group->v1timer) && 
					TV_CMP(group->v1timer, <, now)) {
						TV_CLR(group->v1timer);
				}
				else {
					if (TV_CMP(next, >, group->timeout))
						TV_CPY(next, group->timeout);
					if (TV_SET(group->retransmit) && 
					    TV_CMP(next, >, 
					    group->retransmit))
						TV_CPY(next, group->retransmit);
					if (TV_SET(group->v1timer) && 
					    TV_CMP(next, >, group->v1timer))
						TV_CPY(next, group->v1timer);
				}
			}
			if (del)
				m_Groups.Del(del);
		}
		
		int sleep = (next.tv_sec - now.tv_sec) * 1000;
		sleep += (next.tv_usec - now.tv_usec) / 1000;
		if (next.tv_usec < now.tv_usec)
			sleep += 1000;
		if (m_stopping)
			sleep = 100;
		else if (!sleep)
			continue;
		if (!quiet)
			printf("IGMP main thread: Sleeping %d ms\n", sleep);
		m_CondWait.Wait(sleep);
	}
}

cMulticastGroup* cIgmpMain::FindGroup(in_addr_t Group) const
{
	cMulticastGroup *group = m_Groups.First();
	while (group && group->group != Group)
		group = m_Groups.Next(group);
	return group;
}

void cIgmpMain::IGMPStartGeneralQueryTimer()
{
        m_Querier = true;
        if (m_StartupQueryCount) {
                gettimeofday(&m_GeneralQueryTimer, NULL);
                m_GeneralQueryTimer.tv_sec += IGMP_STARTUP_QUERY_INTERVAL;
                m_StartupQueryCount--;
        }
        else {
                gettimeofday(&m_GeneralQueryTimer, NULL);
                m_GeneralQueryTimer.tv_sec += IGMP_QUERY_INTERVAL;
        }
}

void cIgmpMain::IGMPStartOtherQuerierPresentTimer()
{
        m_Querier = false;
        m_StartupQueryCount = 0;
        gettimeofday(&m_GeneralQueryTimer, NULL);
        m_GeneralQueryTimer.tv_sec += IGMP_OTHER_QUERIER_PRESENT_INTERVAL;
}

void cIgmpMain::IGMPSendGeneralQuery()
{
        m_IgmpListener->IGMPSendQuery(IGMP_ALL_HOSTS, 
		IGMP_QUERY_RESPONSE_INTERVAL);
}

void cIgmpMain::IGMPStartRetransmitTimer(cMulticastGroup* Group)
{
        gettimeofday(&Group->retransmit, NULL);
        TV_ADD(Group->retransmit, IGMP_LAST_MEMBER_QUERY_INTERVAL_TS);
}

void cIgmpMain::IGMPClearRetransmitTimer(cMulticastGroup* Group)
{
        TV_CLR(Group->retransmit);
}

// Group state actions
void cIgmpMain::IGMPStartTimer(cMulticastGroup* Group, in_addr_t Member)
{
	gettimeofday(&Group->timeout, NULL);
	Group->timeout.tv_sec += IGMP_GROUP_MEMBERSHIP_INTERVAL;
	TV_CLR(Group->retransmit);
	Group->reporter = Member;

}

void cIgmpMain::IGMPStartV1HostTimer(cMulticastGroup* Group)
{
	gettimeofday(&Group->v1timer, NULL);
	Group->v1timer.tv_sec += IGMP_GROUP_MEMBERSHIP_INTERVAL;
}

void cIgmpMain::IGMPStartTimerAfterLeave(cMulticastGroup* Group, 
	unsigned int MaxResponseTimeTs)
{
	//Group->Update(time(NULL) + MaxResponseTime
	//* IGMP_LAST_MEMBER_QUERY_COUNT / 10);
	MaxResponseTimeTs *= IGMP_LAST_MEMBER_QUERY_COUNT;
	gettimeofday(&Group->timeout, NULL);
	TV_ADD(Group->timeout, MaxResponseTimeTs);
	TV_CLR(Group->retransmit);
	Group->reporter = 0;
}

void cIgmpMain::IGMPSendGroupQuery(cMulticastGroup* Group)
{
	m_IgmpListener->IGMPSendQuery(Group->group, 
		IGMP_LAST_MEMBER_QUERY_INTERVAL_TS);
}

