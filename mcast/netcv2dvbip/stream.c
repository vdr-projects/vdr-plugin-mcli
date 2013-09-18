#include "dvbipstream.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#endif

#include <pthread.h>
#include <sys/types.h>

#include "clist.h"
#include "stream.h"
#include "thread.h"

#define SLEEPTIME (20*1000)

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

cStream::cStream(int Channum, in_addr_t Addr, int portnum)
		: cThread("udp streamer")
{
	handle = 0;
	channum = Channum;
	addr = Addr;
	m_portnum = portnum;
}

cStream::~cStream(void)
{
}

bool cStream::StartStream(in_addr_t bindaddr)
{
	int rc=0;

	peer.sin_family = AF_INET;
	peer.sin_port = htons(m_portnum);
	peer.sin_addr.s_addr = addr;

	udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udp_socket < 0)
	{
		log_socket_error("Stream: socket()");
		return false;
	}

#if 0
	//----------------------
	// Bind the socket. 
	sockaddr_in m_LocalAddr;

	m_LocalAddr.sin_family = AF_INET;
	m_LocalAddr.sin_port   = htons(m_portnum-1);
	m_LocalAddr.sin_addr.s_addr = bindaddr;

	rc = ::bind( udp_socket, (struct sockaddr*)&m_LocalAddr, 
		sizeof(m_LocalAddr) );
	if ( rc < 0 )
	{
		log_socket_error("STREAM bind()");
		return false;
	}
#endif
#if 0
	char loop = 1;
	rc = setsockopt(udp_socket, IPPROTO_IP, IP_MULTICAST_LOOP, 
		(char*)&loop, sizeof(loop));
	if ( rc < 0 )
	{
		log_socket_error("STREAM setsockopt(IP_MULTICAST_LOOP)");
		return false;
	}
#endif
	rc = setsockopt(udp_socket, IPPROTO_IP, IP_MULTICAST_IF,
		(char *)&bindaddr, sizeof(bindaddr));
	if ( rc < 0 )
	{
		log_socket_error("STREAM setsockopt(IP_MULTICAST_IF)");
		return false;
	}
				

	pthread_mutex_lock(&lock);

	if (handle == 0)
	{
		handle = mcli_stream_setup(channum);
		if (handle)
			Start();
	}
	else
	{
		printf("cStream: handle != NULL !\n");
	}
	pthread_mutex_unlock(&lock);
	
	return true;

}

void cStream::Action()
{
	unsigned int retries;
	size_t len;
	char *ptr;
	struct pollfd p;

	p.fd = udp_socket;
	p.events = POLLOUT;

	while (Running())
	{	
		for (retries=1;;retries++) {
			if (retries&0xff)
				len = mcli_stream_access (handle, &ptr);
			else
				len = mcli_stream_part_access (handle, &ptr);
			if (len)
				break;
			if(!Running())goto out;
			usleep (SLEEPTIME);
		}

		switch (poll(&p,1,100)) {
		case -1:
			log_socket_error( "STREAM poll()" );
		case 0:
			usleep (SLEEPTIME);
			continue;
		default:
			if (!(p.revents&POLLOUT)) {
				usleep (SLEEPTIME);
				continue;
			}
			if (sendto( udp_socket, ptr, len, 0, 
				(struct sockaddr *)&peer, sizeof(peer) ) < 0) {
#ifndef WIN32
				if ((errno == EINTR)||(errno == EWOULDBLOCK)) {
					usleep (SLEEPTIME);
					continue;
				}
#else
				int rc;
				rc = WSAGetLastError();
				if ((rc == WSAEINTR)||(rc == WSAEWOULDBLOCK)) {
					usleep (SLEEPTIME);
					continue;
				}
#endif
				log_socket_error("STREAM: sendto()");
				goto out;
			}
			break;
		}

		mcli_stream_skip (handle);
	}
out:;
}

void cStream::StopStream()
{
	Cancel(3);

	pthread_mutex_lock(&lock);

	if (handle)
	{
		mcli_stream_stop(handle);
		handle = 0;
	}
	else
	{
		printf("cStream: handle == NULL !\n");
	}
	pthread_mutex_unlock(&lock);

#ifdef WIN32
	closesocket(udp_socket);
#else
	close(udp_socket);
#endif

}

