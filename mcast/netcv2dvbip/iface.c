#include "dvbipstream.h"
#include "iface.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#endif

#ifdef WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#endif


#ifndef WIN32
int get_iface_list(struct ifconf *ifconf)
{
   int sock, rval;

   sock = socket(AF_INET,SOCK_STREAM,0);
   if(sock < 0)
   {
     perror("socket");
     return (-1);
   }

   if((rval = ioctl(sock, SIOCGIFCONF , (char*) ifconf  )) < 0 )
     perror("ioctl(SIOGIFCONF)");

   close(sock);

   return rval;
}

int get_iface_ipaddress(struct ifreq *ifreq)
{
   int sock, rval;

   sock = socket(AF_INET,SOCK_STREAM,0);
   if(sock < 0)
   {
     perror("socket");
     return (-1);
   }

   if((rval = ioctl(sock, SIOCGIFADDR , (char*) ifreq  )) < 0 ) {
     //perror("ioctl(SIOCGIFADDR)");
   }
   close(sock);

   return rval;
}

int get_iface_flags(struct ifreq *ifreq)
{
   int sock, rval;

   sock = socket(AF_INET,SOCK_STREAM,0);
   if(sock < 0)
   {
     perror("socket");
     return (-1);
   }

   if((rval = ioctl(sock, SIOCGIFFLAGS , (char*) ifreq  )) < 0 )
     perror("ioctl(SIOCGIFFLAGS)");

   close(sock);

   return rval;
}

int discover_interfaces(iface_t iflist[])
{
   	struct ifreq ifreqs[MAXIF];
   	struct ifconf ifconf;
   	int  nifaces, i;

   	memset(&ifconf,0,sizeof(ifconf));
   	ifconf.ifc_buf = (char*) (ifreqs);
   	ifconf.ifc_len = sizeof(ifreqs);

	if(get_iface_list(&ifconf) < 0) 
  			return 0;
  			
   	nifaces =  ifconf.ifc_len/sizeof(struct ifreq);

	if (!quiet)
   		printf("Interfaces (count = %d):\n", nifaces);

   	for(i = 0; i < nifaces; i++)
   	{
		strncpy(iflist[i].name, ifreqs[i].ifr_name, IFNAMSIZ);
   	
   		u_char *addr;
   		if(get_iface_ipaddress(&ifreqs[i])<0)
   			continue;
		addr = (u_char *) & (((struct sockaddr_in *)&
			(ifreqs[i]).ifr_addr)->sin_addr);
		if (!quiet)
			printf("\t%i - %-10s : addr %d.%d.%d.%d\n",(i+1),
				ifreqs[i].ifr_name,addr[0],addr[1],addr[2],
				addr[3]);
		iflist[i].ipaddr = ( (struct sockaddr_in *)&
			(ifreqs[i]).ifr_addr)->sin_addr.s_addr;
   			
	}
	return nifaces;
}

#endif

#ifdef WIN32

#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

/* Note: could also use malloc() and free() */

int discover_interfaces(iface_t iflist[])
{
    if (!quiet)
	printf("Interfaces:\n");
    /* Declare and initialize variables */

    DWORD dwRetVal = 0;

	unsigned int anum = 0;

    // Set the flags to pass to GetAdaptersAddresses
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;

    // default to IPv4 address family 
    ULONG family = AF_INET;

    LPVOID lpMsgBuf = NULL;

    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    ULONG outBufLen = 0;
    ULONG Iterations = 0;

    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
    PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;

    // Allocate a 15 KB buffer to start with.
    outBufLen = WORKING_BUFFER_SIZE;

    do {

        pAddresses = (IP_ADAPTER_ADDRESSES *) MALLOC(outBufLen);
        if (pAddresses == NULL) {
            printf
                ("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
            return 0;
        }

        dwRetVal =
            GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            FREE(pAddresses);
            pAddresses = NULL;
        } else {
            break;
        }

        Iterations++;

    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));

    if (dwRetVal == NO_ERROR) {
        // If successful, output some information from the data we received
        pCurrAddresses = pAddresses;
        while (pCurrAddresses) {
            if (!quiet) {
                printf("\tIfIndex (IPv4 interface): %u\n", 
			(unsigned int)pCurrAddresses->IfIndex);
                printf("\tAdapter name: %s\n", pCurrAddresses->AdapterName);
            }
			strncpy(iflist[anum].name, pCurrAddresses->AdapterName, 
				IFNAMSIZ);

	    pUnicast = pCurrAddresses->FirstUnicastAddress;

	    if (pCurrAddresses->OperStatus == IfOperStatusUp)
	    {
		if (pUnicast != NULL) {
		    if (!quiet)
			printf("\tUnicast Address: %s\n", 
			    inet_ntoa(((struct sockaddr_in*)
				(pUnicast->Address.lpSockaddr))->sin_addr) );
		    iflist[anum].ipaddr = ((struct sockaddr_in*)
			(pUnicast->Address.lpSockaddr))->sin_addr.S_un.S_addr;
		} else {
                    if (!quiet)
			printf("\tNo Unicast Addresses\n");
                }

                if (!quiet) {
#ifndef __MINGW32__
		    printf("\tDescription: %wS\n", pCurrAddresses->Description);
		    printf("\tFriendly name: %wS\n", 
				pCurrAddresses->FriendlyName);
#else
		    printf("\tDescription: %ls\n", pCurrAddresses->Description);
		    printf("\tFriendly name: %ls\n", 
				pCurrAddresses->FriendlyName);
#endif

		    printf("\tMtu: %lu\n", pCurrAddresses->Mtu);

		    printf("\n");
                }

		anum++;
	    }
            pCurrAddresses = pCurrAddresses->Next;
        }
    } else {
        printf("Call to GetAdaptersAddresses failed with error: %d\n",
               (int)dwRetVal);
        if (dwRetVal == ERROR_NO_DATA)
            printf("\tNo addresses were found for the requested parameters\n");
        else {

            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
                    NULL, dwRetVal, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),   
                    // Default language
                    (LPTSTR) & lpMsgBuf, 0, NULL)) {
                printf("\tError: %s", (char *)lpMsgBuf);
                LocalFree(lpMsgBuf);
                if (pAddresses)
                    FREE(pAddresses);
                return 0;
            }
        }
    }

    if (pAddresses) {
        FREE(pAddresses);
    }

    return 1;
}


#endif


