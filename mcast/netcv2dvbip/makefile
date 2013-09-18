CC=g++
CFLAGS=-O3
LDFLAGS=-s

ARCH= $(shell $(CC) -dumpmachine)
APPLE_DARWIN = $(shell echo $(ARCH) | grep -q 'apple-darwin' && echo "1" || echo "0")
CYGWIN = $(shell echo $(ARCH) | grep -q 'cygwin' && echo "1" || echo "0")
MIPSEL = $(shell echo $(ARCH) | grep -q 'mipsel' && echo "1" || echo "0")

ifeq ($(APPLE_DARWIN), 1)
DEFS:=$(DEFS) -I../common/darwin/include/ -DAPPLE
APPLE=1
CFLAGS:= $(CFLAGS) -fno-common -Wall
else
CFLAGS:= $(CFLAGS) -Wall -Woverloaded-virtual
endif

ifeq ($(MIPSEL),1)
DEFS:=$(DEFS) -DMIPSEL
XML_LIB:=-lxml2
else
XML_INC:=`xml2-config --cflags`
XML_LIB:=`xml2-config --libs`
endif

INCLUDES:=$(INCLUDES) -I../client -I../common $(XML_INC)
DEFS:=$(DEFS) -g -DCLIENT 
LDADD:=$(LDADD) -L../client
STATICLIBS:=$(LIBS) ../client/libmcli.a $(XML_LIB) -lpthread
LIBS:=$(LIBS) $(XML_LIB) -lpthread -lmcli
LDFLAGS:=$(LDFLAGS) -Wl,--as-needed

netcv2dvbip_OBJECTS=main.o parse.o mclilink.o siparser.o crc32.o clist.o stream.o thread.o misc.o streamer.o igmp.o iface.o

all: netcv2dvbip netcv2dvbip-static

MAKEDEP = $(CC) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): makefile
	$(MAKEDEP) $(INCLUDES) $(netcv2dvbip_OBJECTS:%.o=%.c) > $@

-include $(DEPFILE)

netcv2dvbip: $(netcv2dvbip_OBJECTS) ../client/libmcli.so
	$(CC) $(LDFLAGS) $(netcv2dvbip_OBJECTS) $(LDADD) $(LIBS) -o $@

netcv2dvbip-static: $(netcv2dvbip_OBJECTS) ../client/libmcli.a
	$(CC) $(LDFLAGS) $(netcv2dvbip_OBJECTS) $(LDADD) $(STATICLIBS) -o $@

../client/libmcli.so: ../client/libmcli.a

../client/libmcli.a:
	make -C ../client

.c.o:
	$(CC) $(DEFS) $(INCLUDES)  $(CFLAGS) -c $<


clean:
	$(RM) -f $(DEPFILE) *.o *~ netcv2dvbip netcv2dvbip-static
	
