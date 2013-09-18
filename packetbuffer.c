/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#include <vdr/plugin.h>
#include <sys/time.h>
#include "packetbuffer.h"

uint64_t Now (void)
{
#if 0
	struct timeval t;
        if (gettimeofday (&t, NULL) == 0)
                return (uint64_t (t.tv_sec)) * 1000 + t.tv_usec / 1000;
        return 0;
#else
        return clock();
#endif
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
cMyPacketBuffer::cMyPacketBuffer (int Size, int Packets)
{
	if (Packets == 0)
		Packets = Size / 2048;

	// Make Packets a power of 2 to avoid expensive modulo
	int n = 1;
	for (int i = 0; i < 16; i++) {
		if (n >= Packets) {
			Packets = n;
			break;
		}
		n <<= 1;
	}

	dataBuffer = (uchar *) malloc (Size);
	memset (dataBuffer, 0, Size);
	posBuffer = (posData *) malloc (Packets * sizeof (posData));
	pthread_mutex_init (&m_lock, NULL);

	posSize = Packets;
	dataSize = Size;
	memset (posBuffer, 0, Packets * sizeof (posData));
	rp = wp = 0;
	posRead = NULL;
	posWrite = NULL;
	posReadNum = 0;
	invalidate = 0;
	putTimeout = getTimeout = 0;
}

//--------------------------------------------------------------------------
cMyPacketBuffer::~cMyPacketBuffer (void)
{
	free (dataBuffer);
	free (posBuffer);
}

//--------------------------------------------------------------------------
int cMyPacketBuffer::FindSpace (int size)
{
	int wpm = (wp - 1) & (posSize - 1);
	posData *pr, *pw;

	if (wpm < 0)
		wpm += posSize;

	if (rp == wp) {
		if (size > dataSize)
			return -1;
		return 0;
	}
	pr = posBuffer + rp;
	pw = posBuffer + wpm;

	if (pr->offset <= pw->offset) {
		if (pw->offset + pw->realSize + size < dataSize) {
			return pw->offset + pw->realSize;
		}
		if (size < pr->offset)
			return 0;
		return -1;
	} else {
		if (pw->offset + pw->realSize + size < dataSize)
			return pw->offset + pw->realSize;
		return -1;
	}
	return -1;
}

//--------------------------------------------------------------------------
uchar *cMyPacketBuffer::PutStart (int size)
{
	uint64_t starttime = 0;
	int offset;
	int nwp;
	int rsize;
	pthread_mutex_lock (&m_lock);
//      rsize= (size+15)&~15;
	rsize = size;
	while (true) {
		offset = FindSpace (rsize);
		if (offset != -1)
			break;
		if (putTimeout && !starttime)
			starttime = Now ();
		if (!putTimeout || (Now () - starttime) > (uint64_t) (putTimeout)) {
			pthread_mutex_unlock (&m_lock);
			return NULL;
		}
		usleep (5 * 1000);
	}
	nwp = (wp) & (posSize - 1);

	posWrite = posBuffer + nwp;
	posWrite->offset = offset;
	posWrite->realSize = rsize;

//      printf("PUTSTART wp %i, start %x\n",nwp, offset);
	return dataBuffer + offset;
}

//--------------------------------------------------------------------------
void cMyPacketBuffer::PutEnd (int size, int flags, uint64_t timestamp)
{
	if (!posWrite)
		return;

	if (size > posWrite->realSize)
		size = posWrite->realSize;

	posWrite->size = size;
	posWrite->flags = flags;
	posWrite->timestamp = timestamp;
	wp = (wp + 1) & (posSize - 1);
	pthread_mutex_unlock (&m_lock);

}

//--------------------------------------------------------------------------
uchar *cMyPacketBuffer::GetStartSub (int *readp, int timeout, int *size, int *flags, uint64_t * timestamp)
{
	uint64_t starttime = 0;

	if (*readp == wp && timeout)
		starttime = Now ();
//      printf("GET rp %i wp %i\n",readp,wp);
	while (*readp == wp) {
		if (!timeout || (Now () - starttime) > (uint64_t) (timeout))
			return 0;
		usleep (20 * 1000);
	}
#if 0
	if (readp > posSize) {
		// Fixme sync
		return 0;
	}
#endif
	posRead = posBuffer + *readp;

	if (flags)
		*flags = posRead->flags;
	if (size)
		*size = posRead->size;
	if (timestamp)
		*timestamp = posRead->timestamp;
//      printf("GET rp %i, offset %x\n",readp,posRead->offset);
	return dataBuffer + posRead->offset;
}

//--------------------------------------------------------------------------
uchar *cMyPacketBuffer::GetStart (int *size, int *flags, uint64_t * timestamp)
{
	if (posRead) {
#if 1
		if (flags)
			*flags = posRead->flags;
		if (size)
			*size = posRead->size;
		return dataBuffer + posRead->offset;
#else
		GetEnd ();
#endif
	}

	if (invalidate) {
		rp = wp;
		invalidate = 0;
		return 0;
	}
	posReadNum = 1;
	return GetStartSub (&rp, getTimeout, size, flags, timestamp);
}

//--------------------------------------------------------------------------
void cMyPacketBuffer::GetEnd (void)
{
	if (!posRead)
		return;
	rp = (rp + posReadNum) & (posSize - 1);
	posRead = NULL;
	posReadNum = 0;
}

//--------------------------------------------------------------------------
// Try to get multiple PES at once
uchar *cMyPacketBuffer::GetStartMultiple (int maxsize, int *size, int *flags, uint64_t * timestamp)
{
	uchar *buf, *lastbuf, *startbuf;
	int sz, fl;
	int readp, packets;
	int totalsize;
	int startflags;
	int timeout = getTimeout;
	uint64_t tsp, starttsp;
#if 0
	if (posRead)
		GetEnd ();
#endif
//printf("fill %d    \r", (wp>=rp) ? wp-rp : posSize-rp+wp);
	if (invalidate) {
		rp = wp;
		invalidate = 0;
		return 0;
	}
	readp = rp;
	startbuf = NULL;
	lastbuf = NULL;
	totalsize = 0;
	packets = 0;
	startflags = 0;
	starttsp = 0;
	while (1) {
		sz = 0;
		buf = GetStartSub (&readp, timeout, &sz, &fl, &tsp);
//              printf("GOT %x %i\n",buf,sz);
		if (!startbuf) {
			if (!buf)
				return NULL;
			startbuf = buf;
			startflags = fl;
			starttsp = tsp;
		} else {
			if (lastbuf + sz != buf ||	// Buffer wraparound or no buffer
			    (totalsize + sz) > maxsize ||	// 
			    fl != 0) {	// packet start
				if (size)
					*size = totalsize;
				if (flags)
					*flags = startflags;
				if (timestamp)
					*timestamp = starttsp;
				posReadNum = packets;
				return startbuf;
			}
		}
		readp = (readp + 1) & (posSize - 1);
		packets++;
		totalsize += sz;
		lastbuf = buf;
		timeout = 0;
	}
	return NULL;
}

//--------------------------------------------------------------------------
void cMyPacketBuffer::SetTimeouts (int PutTimeout, int GetTimeout)
{
	putTimeout = PutTimeout;
	getTimeout = GetTimeout;
}

//--------------------------------------------------------------------------
void cMyPacketBuffer::Invalidate (void)
{
	invalidate = 1;
}
