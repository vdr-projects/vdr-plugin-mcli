#Comment this out to disable debugging output
#DEBUG = 1
API_SOCK=1

ifdef RBMINI
  ARMEL=1
endif

APPLE_DARWIN = $(shell gcc -dumpmachine | grep -q 'apple-darwin' && echo "1" || echo "0")
CYGWIN = $(shell gcc -dumpmachine | grep -q 'cygwin' && echo "1" || echo "0")

DEFINES = -DCLIENT -D_REENTRANT -D_GNU_SOURCE

ifeq ($(CYGWIN), 1)
WIN32=1
else
API_SOCK=1
endif

ifeq ($(APPLE_DARWIN), 1)
INCLUDES += -I../common/darwin/include/
DEFINES  += -DAPPLE
APPLE=1
endif

VDRDIR=../../../../..
-include $(VDRDIR)/Make.config

ifdef ARMEL
  XML_INC := -I/usr/arm-linux-gnueabi/include/libxml2
  XML_LIB := -lxml2
else
  XML_INC := `xml2-config --cflags`
  XML_LIB := `xml2-config --libs`
  LIBRARY_PATH = /usr/lib
endif

CFLAGS ?= -Os -Wall -pthread 

INCLUDES += $(XML_INC) -I../dvbloop -I../common/ -I../client

LDFLAGS:=$(XML_LIB) -pthread

ifdef API_SHM
LDFLAGS:= $(LDFLAGS) -lrt
CFLAGS:= $(CFLAGS) -DAPI_SHM
endif

ifdef API_SOCK
CFLAGS:= $(CFLAGS) -DAPI_SOCK
endif

ifdef DEBUG
LDFLAGS:= $(LDFLAGS) -g
CFLAGS:= $(CFLAGS) -g -DDEBUG
endif

ifdef WIN32
CFLAGS:= $(CFLAGS) -DWIN32
endif



NETCVDIAG = netcvdiag
NETCVDIAG_OBJS = netcvdiag.o tools.o

NETCVUPDATE = netcvupdate
NETCVUPDATE_OBJS = netcvupdate.o

NETCVLOGVIEW = netcvlogview
NETCVLOGVIEW_OBJS = netcvlogview.o mcast.o

OBJS := $(NETCVDIAG_OBJS) $(NETCVUPDATE_OBJS) $(NETCVLOGVIEW_OBJS)

all: $(NETCVDIAG) $(NETCVUPDATE) $(NETCVLOGVIEW)

static: $(NETCVDIAG)-static $(NETCVUPDATE)-static


MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)


$(NETCVDIAG): $(NETCVDIAG_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(NETCVDIAG_OBJS) $(LDLIBS)
ifndef DEBUG
ifndef WIN32
	strip $(NETCVDIAG)
endif
endif

$(NETCVUPDATE): $(NETCVUPDATE_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(NETCVUPDATE_OBJS) $(LDLIBS)
ifndef DEBUG
ifndef WIN32
	strip $(NETCVUPDATE)
endif
endif

$(NETCVLOGVIEW): $(NETCVLOGVIEW_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(NETCVLOGVIEW_OBJS) $(LDLIBS)
ifndef DEBUG
ifndef WIN32
	strip $(NETCVLOGVIEW)
endif
endif

$(NETCVDIAG)-static: $(NETCVDIAG_OBJS)
	$(CC) $(LDFLAGS) -static -static-libgcc -o $@ $(NETCVDIAG_OBJS) $(LDLIBS) $(LIBRARY_PATH)/libm.a  $(LIBRARY_PATH)/libz.a
	strip $(NETCVDIAG)-static

$(NETCVUPDATE)-static: $(NETCVUPDATE_OBJS)
	$(CC) $(LDFLAGS) -static -static-libgcc -o $@ $(NETCVUPDATE_OBJS) $(LDLIBS) $(LIBRARY_PATH)/libm.a
	strip $(NETCVUPDATE)-static

$(NETCVLOGVIEW)-static: $(NETCVLOGVIEW_OBJS)
	$(CC) $(LDFLAGS) -static -static-libgcc -o $@ $(NETCVLOGVIEW_OBJS) $(LDLIBS) $(LIBRARY_PATH)/libm.a  $(LIBRARY_PATH)/libz.a $(LIBRARY_PATH)/libpthread.a
	strip $(NETCVLOGVIEW)-static

install: $(NETCVDIAG) $(NETCVUPDATE) $(NETCVLOGVIEW)
	install -p $(NETCVDIAG) /usr/sbin/$(NETCVDIAG)
	install -p $(NETCVUPDATE) /usr/sbin/$(NETCVUPDATE)
	install -p $(NETCVLOGVIEW) /usr/sbin/$(NETCVLOGVIEW)

depend: .dependencies
	#makedepend -Y -- $(CFLAGS)  -- *c >/dev/null 2>&1

clean:
	rm -f $(NETCVDIAG) $(NETCVUPDATE) $(NETCVLOGVIEW) *.elf *.gdb *.o *~

%.o: %.c
	$(CC) -c $(CFLAGS) $(INCLUDES) $(DEFINES) -o $@ $<

