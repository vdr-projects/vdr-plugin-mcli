#ifndef __MISC_H
#define __MISC_H

#ifdef _MSC_VER
   typedef unsigned __int64 uint64_t;	// Define it from MSVC's internal type
#else
   #include <stdint.h>		    // Use the C99 official header
#endif

#define bool int

#ifndef __STL_CONFIG_H // in case some plugin needs to use the STL
template<class T> inline T xmin(T a, T b) { return a <= b ? a : b; }
template<class T> inline T xmax(T a, T b) { return a >= b ? a : b; }
template<class T> inline int xsgn(T a) { return a < 0 ? -1 : a > 0 ? 1 : 0; }
template<class T> inline void xswap(T &a, T &b) { T t = a; a = b; b = t; }
#endif

#ifdef WIN32
typedef unsigned long in_addr_t;
#define SCKT SOCKET
#else
#include <arpa/inet.h>
#define SCKT int
#endif

#ifndef NULL
#define NULL 0
#endif

#ifdef WIN32
#define strdup _strdup
#endif

#define HI_BYTE(byte)    (((byte) >> 4) & 0x0F)
#define LO_BYTE(byte)    ((byte) & 0x0F)

extern void log_socket_error(const char* msg);
extern bool GetAbsTime(struct timespec *Abstime, int MillisecondsFromNow);

#ifdef WIN32
struct timezone {
               int tz_minuteswest;     /* minutes west of Greenwich */
               int tz_dsttime;         /* type of DST correction */
};
extern int gettimeofday(struct timeval * tp, struct timezone * tzp);
bool IsUserAdmin( bool* pbAdmin );
bool IsVistaOrHigher();
#else
#include <sys/time.h>
#endif

class cTimeMs {
private:
  uint64_t begin;
public:
  cTimeMs(int Ms = 0);
      ///< Creates a timer with ms resolution and an initial timeout of Ms.
  static uint64_t Now(void);
  void Set(int Ms = 0);
  bool TimedOut(void);
  uint64_t Elapsed(void);
};

extern int quiet;

#endif
