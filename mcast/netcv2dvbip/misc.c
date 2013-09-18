#include <stdio.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#else
#include <sys/time.h>
#endif

#include <pthread.h>
#include "misc.h"

void log_socket_error(const char* msg)
{
#ifdef WIN32
        printf(msg);
        printf(": WSAGetLastError(): %d",  WSAGetLastError());
#else
        perror(msg);
#endif
}

#ifdef WIN32
static const unsigned __int64 epoch = 116444736000000000;

int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
	FILETIME	file_time;
	SYSTEMTIME	system_time;
	ULARGE_INTEGER ularge;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	ularge.LowPart = file_time.dwLowDateTime;
	ularge.HighPart = file_time.dwHighDateTime;

	tp->tv_sec = (long) ((ularge.QuadPart - epoch) / 10000000L);
	tp->tv_usec = (long) (system_time.wMilliseconds * 1000);

	return 0;
}
#endif

bool GetAbsTime(struct timespec *Abstime, int MillisecondsFromNow)
{
	struct timeval now;

	if (!gettimeofday(&now, NULL)) {
		now.tv_sec  += MillisecondsFromNow / 1000;
		now.tv_usec += (MillisecondsFromNow % 1000) * 1000;
		if (now.tv_usec >= 1000000) {
			now.tv_sec++;
			now.tv_usec -= 1000000;
		}
		Abstime->tv_sec = now.tv_sec;
		Abstime->tv_nsec = now.tv_usec * 1000;
		return true;
	}
	return false;
}

// --- cTimeMs ---------------------------------------------------------------

cTimeMs::cTimeMs(int Ms)
{
	Set(Ms);
}

uint64_t cTimeMs::Now(void)
{
	struct timeval t;

	if (gettimeofday(&t, NULL) == 0)
		return (uint64_t(t.tv_sec)) * 1000 + t.tv_usec / 1000;
	return 0;
}

void cTimeMs::Set(int Ms)
{
	begin = Now() + Ms;
}

bool cTimeMs::TimedOut(void)
{
	return Now() >= begin;
}

uint64_t cTimeMs::Elapsed(void)
{
	return Now() - begin;
}

#ifdef WIN32

#ifdef __MINGW32__
#define __try if(1)
#define __leave goto out
#define __finally if(1)
#endif

bool IsUserAdmin( bool* pbAdmin )
{
#if WINVER < 0x0500
	HANDLE hAccessToken = NULL;
	PBYTE pInfoBuffer = NULL;
	DWORD dwInfoBufferSize = 1024; // starting buffer size
	PTOKEN_GROUPS ptgGroups = NULL;
#endif
	PSID psidAdministrators = NULL;
	SID_IDENTIFIER_AUTHORITY siaNtAuthority = {SECURITY_NT_AUTHORITY};
	BOOL bResult = FALSE;

__try
{

	// initialization of the security id
	if( !AllocateAndInitializeSid(
		&siaNtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS, 0,0,0,0,0,0, &psidAdministrators ) )
			__leave;

#if WINVER < 0x0500
	// code for the OS < W2k
	// open process token
	if( !OpenProcessToken( GetCurrentProcess(),TOKEN_QUERY,&hAccessToken ) )
		__leave;

	do // lets make a buffer for the information from the token
	{
		if( pInfoBuffer )
		delete pInfoBuffer;
		pInfoBuffer = new BYTE[dwInfoBufferSize];
		if( !pInfoBuffer )
			__leave;
		SetLastError( 0 );
		if( !GetTokenInformation( hAccessToken,	TokenGroups, 
			pInfoBuffer, dwInfoBufferSize, &dwInfoBufferSize ) &&
			( ERROR_INSUFFICIENT_BUFFER != GetLastError() ) )
				__leave;
		else
			ptgGroups = (PTOKEN_GROUPS)pInfoBuffer;
	}
	while( GetLastError() ); // if we encounter an error, then the 
				 // initializing buffer too small, 
				 // lets make it bigger

	// lets check the security id one by one, while we find one we need
	for( UINT i = 0; i < ptgGroups->GroupCount; i++ )
	{
		if( EqualSid(psidAdministrators,ptgGroups->Groups[i].Sid) )
		{
			*pbAdmin = TRUE;
			bResult = TRUE;
			__leave;
		}
	}
#else
	// this is code for the W2K
	bResult = CheckTokenMembership( NULL, psidAdministrators, pbAdmin );
#endif
}

__finally
{
#ifdef __MINGW32__
out:
#endif
#if WINVER < 0x0500
	if( hAccessToken )
		CloseHandle( hAccessToken );
	if( pInfoBuffer )
		delete pInfoBuffer;
#endif
	if( psidAdministrators )
		FreeSid( psidAdministrators );
}

return bResult;
}

bool IsVistaOrHigher()
{
	OSVERSIONINFO osvi;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	GetVersionEx(&osvi);

	return (osvi.dwMajorVersion >= 6);
}

#endif
