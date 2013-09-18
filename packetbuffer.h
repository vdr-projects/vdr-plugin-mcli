/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#ifndef VDR_MCLI_PACKETBUFFER_H
#define VDR_MCLI_PACKETBUFFER_H

#define USE_VDR_PACKET_BUFFER
#ifdef USE_VDR_PACKET_BUFFER
#include <vdr/ringbuffer.h>
#endif
//--------------------------------------------------------------------------
// Packetized Buffer
//--------------------------------------------------------------------------

typedef struct
{
	int offset;
	int size;
	int realSize;		// incl. alignment
	int flags;
	uint64_t timestamp;
} posData;

class cMyPacketBuffer
{
	int dataSize;
	int posSize;
	uchar *dataBuffer;
	posData *posBuffer;
	int rp, wp;
	posData *posRead, *posWrite;
	int posReadNum;
	int invalidate;
	int putTimeout, getTimeout;
	pthread_mutex_t m_lock;

	int FindSpace (int size);

	uchar *GetStartSub (int *readp, int timeout, int *size, int *flags, uint64_t * timestamp);
      public:
	  cMyPacketBuffer (int Size, int Packets);
	 ~cMyPacketBuffer ();
	uchar *PutStart (int size);
	void PutEnd (int size, int flags, uint64_t timestamp);
	uchar *GetStart (int *size, int *flags, uint64_t * timestamp);
	uchar *GetStartMultiple (int maxsize, int *size, int *flags, uint64_t * timestamp);
	void GetEnd (void);
//      void   GetEndMultiple(void);
	void Invalidate (void);
	void SetTimeouts (int PutTimeout, int GetTimeout);
};
#endif
