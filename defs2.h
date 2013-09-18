/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#ifndef __DEFS_H__
#define __DEFS_H__

#ifdef WIN32
#ifdef __CYGWIN__
#include <cygwin/version.h>
#include <cygwin/in.h>
#include <cygwin/socket.h>
#else
#define _CRT_SECURE_NO_WARNINGS
#define _WIN32_WINNT 0x0502
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>

#define _SOTYPE char*
#define IFNAMSIZ 1024
#define CA_TPDU_MAX 2048
#define _POSIX_PATH_MAX MAX_PATH
#define usleep(useconds) Sleep((useconds+500)/1000)
#define sleep(seconds) Sleep((seconds)*1000)
#define EAFNOSUPPORT WSAEAFNOSUPPORT
#ifndef IP_ADAPTER_IPV6_ENABLED
#define IP_ADAPTER_IPV6_ENABLED  0x0100
#endif

int inet_pton(int af, const char *src, void *dst);
const char *inet_ntop(int af, const void *src, char *dst, size_t size);
int inet_aton(const char *cp, struct in_addr *addr);
int getopt(int nargc, char **nargv, char *ostr);
extern int opterr, optind, optopt, optreset;
extern char *optarg;

typedef struct
{
    DWORD thread;
    HANDLE threadH;             /* Win32 thread handle - POSIX thread is invalid if threadH == 0 */
} ptw32_thread_t;

typedef unsigned int uint32_t;
typedef uint32_t __u32;
typedef uint32_t u_int32_t;
typedef unsigned short uint16_t;
typedef uint16_t __u16;
typedef uint16_t u_int16_t;
typedef unsigned char uint8_t;
typedef uint8_t __u8;
typedef uint8_t u_int8_t;
#ifndef s6_addr16
#define s6_addr16	s6_words
#endif
#if ! defined _GNU_SOURCE && defined __cplusplus
#define CALLCONV	extern "C"
#else
#define CALLCONV
#endif
#ifdef LIBRARY
#define DLL_SYMBOL CALLCONV __declspec( dllexport )
#else
#define DLL_SYMBOL CALLCONV __declspec( dllimport )
#endif

#define pthread_exist(x) (x).p
#define pthread_null(x) (x).p=NULL
#define _SOTYPE char*
#define INET6
#define API_WIN
#define LIBXML_STATIC
#define PTW32_STATIC_LIB
#define MULTI_THREAD_RECEIVER
#endif
#else
#if defined __cplusplus
#define CALLCONV	extern "C"
#else
#define CALLCONV
#endif
#define DLL_SYMBOL CALLCONV
#define pthread_exist(x) x
#define pthread_null(x) x=0
#define _SOTYPE void*
#define SOCKET int

#if ! (defined __uClinux__ || defined APPLE || defined MIPSEL)
#include <mcheck.h>
#include <ifaddrs.h>
#endif
#include <pwd.h>
#include <sched.h>
#include <syslog.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include <termios.h>

#include <arpa/inet.h>
#ifndef APPLE
#include <linux/version.h>
#include <netpacket/packet.h>
#include <sys/sysinfo.h>
#else
typedef unsigned int uint32_t;
typedef uint32_t __u32;
typedef uint32_t u_int32_t;
typedef unsigned short uint16_t;
typedef uint16_t __u16;
typedef uint16_t u_int16_t;
typedef unsigned char uint8_t;
typedef uint8_t __u8;
typedef uint8_t u_int8_t;

#define CA_TPDU_MAX 2048

#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#ifndef s6_addr16
#define s6_addr16 __u6_addr.__u6_addr16
#endif
#endif

#include <netdb.h>

#include <net/if.h>
#ifdef APPLE
#include <ifaddrs.h>
#include <net/if_types.h>
#endif
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/icmp6.h>
#include <netinet/ip_icmp.h>
#include <netinet/if_ether.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>            /* for iovec{} and readv/writev */
#include <sys/un.h>             /* for Unix domain sockets */
#include <sys/utsname.h>
#include <sys/wait.h>

#if defined __uClinux__
#include <mathf.h>
#endif
#define closesocket close
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zlib.h>

#include <sys/stat.h>

//----------------------------------------------------------------------
#ifndef __uClinux__
  //DVBAPI
#include <linux/dvb/frontend.h>
#include <linux/dvb/ca.h>
#if ! (defined WIN32 || defined APPLE)
#include <linux/dvb/dmx.h>
#endif
//      #else
//      #endif

#define dvb_ioctl ioctl
#define dvb_close close
#else
#include <dvb/frontend.h>
#include <ci/ca.h>
#endif // __uClinux__

#define CA_TPDU_MAX 2048

typedef struct recv_sec
{
    struct dvb_diseqc_master_cmd diseqc_cmd;
    fe_sec_mini_cmd_t mini_cmd;
    fe_sec_tone_mode_t tone_mode;
    fe_sec_voltage_t voltage;
} recv_sec_t;

#define CA_MAX_SLOTS 16
typedef struct
{
    ca_caps_t cap;
    ca_slot_info_t info[CA_MAX_SLOTS];
} recv_cacaps_t;

typedef struct recv_festatus
{
    fe_status_t st;
    uint32_t ber;
    uint16_t strength;
    uint16_t snr;
    uint32_t ucblocks;
} recv_festatus_t;

//XML
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#if ! defined GETTID && ! defined WIN32 && ! defined APPLE
#include <asm/unistd.h>
#define gettid() syscall (__NR_gettid)
#else
#define gettid pthread_self
#endif

#define UUID_SIZE 256


#ifndef WIN32

#ifdef SYSLOG
extern char *_logstr;
extern pthread_mutex_t _loglock;

#ifdef DEBUG
#define dbg(format, arg...) { pthread_mutex_lock (&_loglock); sprintf(_logstr, "%s:%d " format , __FILE__ , __LINE__ , ## arg); syslog_write(_logstr); pthread_mutex_unlock (&_loglock);}
#else
#define dbg(format, arg...) do {} while (0)
#endif

#define err(format, arg...) {pthread_mutex_lock (&_loglock); sprintf(_logstr, "err:%s:%d: %s (%d): " format , __FILE__ , __LINE__ ,strerror(errno), errno, ## arg); fprintf(stdout, "%s", _logstr); syslog_write(_logstr);abort(); pthread_mutex_unlock (&_loglock);}
#define info(format, arg...){pthread_mutex_lock (&_loglock); sprintf(_logstr, format ,## arg); fprintf(stdout, "%s", _logstr); syslog_write(_logstr); pthread_mutex_unlock (&_loglock);}
#define warn(format, arg...){pthread_mutex_lock (&_loglock); sprintf(_logstr, format ,## arg); fprintf(stdout, "%s", _logstr); syslog_write(_logstr); pthread_mutex_unlock (&_loglock);}
#define sys(format, arg...){pthread_mutex_lock (&_loglock); sprintf(_logstr, format ,## arg); syslog_write(_logstr); pthread_mutex_unlock (&_loglock);}
#elif defined DEBUG
#define dbg(format, arg...) {printf("%s:%d " format , __FILE__ , __LINE__ , ## arg)}
#define err(format, arg...) {fprintf(stderr,"err:%s:%d: %s (%d): " format , __FILE__ , __LINE__ ,strerror(errno), errno, ## arg);print_trace();abort();}
#define info(format, arg...) printf("%s:%d: " format , __FILE__ , __LINE__ ,## arg)
#define warn(format, arg...) fprintf(stderr,"%s:%d: " format , __FILE__ , __LINE__ ,## arg)
#else
#define dbg(format, arg...) do {} while (0)
#define err(format, arg...) {fprintf(stderr,"%s (%d): " format, strerror(errno), errno, ## arg);exit(-1);}
#define info(format, arg...) printf(format , ## arg)
#define warn(format, arg...) fprintf(stderr, format , ## arg)
#define sys(format, arg...) printf(format, ## arg)
#endif // Syslog

#else // WIN32
#ifdef DEBUG
static void inline dbg(char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    printf("%s:%d %s", __FILE__, __LINE__, buffer);
    va_end(args);
}
static void inline err(char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    fprintf(stderr, "err:%s:%d: %s (%d): %s ", __FILE__, __LINE__,
            strerror(errno), errno, buffer);
    va_end(args);
    abort();
}
static void inline info(const char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    printf("%s:%d: %s", __FILE__, __LINE__, buffer);
    va_end(args);
}
static void inline warn(const char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    fprintf(stderr, "%s:%d: %s", __FILE__, __LINE__, buffer);
    va_end(args);
}
#else
static void inline dbg(char *format, ...)
{
}
static void inline err(char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    fprintf(stderr, "err:%s:%d: %s", strerror(errno), errno, buffer);
    va_end(args);
    abort();
}
static void inline info(const char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    puts(buffer);
    va_end(args);
}
static void inline warn(const char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    fputs(buffer, stderr);
    va_end(args);
}

#endif //DEBUG
#endif // WIN32

#ifndef MICROBLAZE
#define FE_DVBS2 (FE_ATSC+1)
#endif

// RMM S2 Extension
#define FEC_1_4 10
#define FEC_1_3 11
#define FEC_2_5 12
#define FEC_3_5 13
#define FEC_9_10 14
#define QPSK_S2 9
#define PSK8 10

#ifdef MICROBLAZE
#define STATIC
#else
#define STATIC static
#endif
#endif
