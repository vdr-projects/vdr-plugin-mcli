#ifndef __IFACE_H
#define __IFACE_H

#include "defs.h"

#include "misc.h"

// iface.c

#define MAXIF 20

typedef struct
{
        char name[IFNAMSIZ];
        in_addr_t ipaddr;
} iface_t;

int discover_interfaces(iface_t *iflist);


#endif

