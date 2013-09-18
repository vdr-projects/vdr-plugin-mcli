#Comment this out to disable debugging output
#DEBUG=1
#VERBOSE=1
#WIN32=1
#API_SOCK=1
#VERBOSE=1
#BACKTRACE=1

ifdef RBMINI
  ARMEL=1
endif

ARCH= $(shell $(CC) -dumpmachine)
APPLE_DARWIN = $(shell echo $(ARCH) | grep -q 'apple-darwin' && echo "1" || echo "0")
CYGWIN = $(shell echo $(ARCH) | grep -q 'cygwin' && echo "1" || echo "0")
MIPSEL = $(shell echo $(ARCH) | grep -q 'mipsel' && echo "1" || echo "0")

DEFS=-DCLIENT -DLIBRARY -D_REENTRANT -D_GNU_SOURCE

ifeq ($(CYGWIN), 1)
WIN32=1
else
API_SOCK=1
endif

ifeq ($(APPLE_DARWIN), 1)
DEFS:=$(DEFS) -I../common/darwin/include/ -DAPPLE
APPLE=1
endif

VDRDIR=../../../../..
-include $(VDRDIR)/Make.config

ifdef ARMEL
  XML_INC := -I/usr/arm-linux-gnueabi/include/libxml2
  XML_LIB := -lxml2
  CROSS = arm-linux-gnueabi-
else
ifeq ($(MIPSEL),1)
DEFS:=$(DEFS) -DMIPSEL
XML_INC:=-I../../libxml2/include
XML_LIB:=-L../../libxml2/lib
else
XML_INC:=`xml2-config --cflags`
XML_LIB:=`xml2-config --libs`
LIBRARY_PATH=/usr/lib
endif
endif
ifeq ($(APPLE_DARWIN), 1)
CFLAGS:= $(CFLAGS) -fPIC -fno-common -Wall -I../common $(DEFS)
else
CFLAGS:= $(CFLAGS) -fPIC -Wall -I../common $(DEFS)
endif

ifdef BACKTRACE
CFLAGS:= $(CFLAGS) -DBACKTRACE -g
endif

ifdef API_SHM
LDFLAGS:= $(LDFLAGS) -lrt
CFLAGS:= $(CFLAGS) -DAPI_SHM
endif
ifdef API_SOCK
CFLAGS:= $(CFLAGS) -DAPI_SOCK
endif

ifdef VERBOSE
CFLAGS:= $(CFLAGS) -DDEBUG
DEBUG=1
endif

ifdef WIN32
CFLAGS:= -Iwin32/include $(CFLAGS) -mno-cygwin -fPIC -DWIN32
LDFLAGS:= -Lwin32/lib $(LDFLAGS) -mno-cygwin
LDLIBS:= -lpthreadGC2 -lxml2 -lz -lws2_32 -liphlpapi 
else
CFLAGS:= $(CFLAGS)  -I../dvbloop $(XML_INC)
LDFLAGS:=$(LDFLAGS)
LDLIBS:=$(XML_LIB) -lpthread -lz -lm
endif

ifdef DEBUG
LDFLAGS:= $(LDFLAGS) -g -rdynamic
CFLAGS:= $(CFLAGS) -g -O0
else
CFLAGS:= $(CFLAGS) -O3
endif

MCLI = mcli

MCLI_OBJS= mld_common.o mld_client.o mld_reporter.o mcast.o recv_tv.o recv_ccpp.o tools.o tca_handler.o tra_handler.o satlists.o interfaces.o api_server.o ciparser.o ci_handler.o mmi_handler.o
ifdef WIN32
MCLI_OBJS := $(MCLI_OBJS) inet_pton.o inet_ntop.o inet_aton.o
else
MCLI_OBJS := $(MCLI_OBJS)
endif

MCLI_SOBJS = main.o
ifdef WIN32
MCLI_SOBJS := $(MCLI_SOBJS) dummy_client.o
else
ifdef APPLE
MCLI_SOBJS := $(MCLI_SOBJS) dummy_client.o input.o
else
MCLI_SOBJS := $(MCLI_SOBJS) dvblo_handler.o input.o
endif 
endif

all: lib$(MCLI)

static: $(MCLI)-static


MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	$(MAKEDEP) $(CFLAGS) $(MCLI_OBJS:%.o=%.c) > $@

-include $(DEPFILE)


$(MCLI): $(MCLI_OBJS) $(MCLI_SOBJS)
	$(CC) $(LDFLAGS) -o $@ $(MCLI_OBJS) $(MCLI_SOBJS) $(LDLIBS)
ifndef DEBUG
ifndef WIN32
	strip $(MCLI)
endif
endif

lib$(MCLI): $(MCLI_OBJS)
ifdef WIN32
	$(CC) $(LDFLAGS) -shared -o $@.dll $(MCLI_OBJS) $(LDLIBS) -Wl,--out-implib,$@.a -Wl,--output-def,$@.def
	lib /def:$@.def /machine:x86 /out:..\\common\\win32\\lib\\$@.lib
#	dlltool -k --dllname $@.dll --output-lib win32/lib/$@.lib --def $@.def
	cp -a $@.dll win32/lib/
	cp -a $@.a win32/lib/
	cp -a $@.def win32/lib/
endif
ifdef APPLE
	$(CC) $(LDFLAGS) -dynamiclib -o $@.dylib $(MCLI_OBJS) $(LDLIBS)
	$(AR) $(ARFLAGS) $@.a $(MCLI_OBJS)
else
	$(CC) $(LDFLAGS) -shared -o $@.so $(MCLI_OBJS) $(LDLIBS)
	$(AR) $(ARFLAGS) $@.a $(MCLI_OBJS)
endif


$(MCLI)-shared: lib$(MCLI)
	$(CC) -c $(CFLAGS) -ULIBRARY -o main.o main.c
ifdef WIN32
	$(CC) -c $(CFLAGS) -ULIBRARY -o dummy_client.o dummy_client.c
else
ifdef APPLE
	$(CC) -c $(CFLAGS) -ULIBRARY -o input.o input.c
	$(CC) -c $(CFLAGS) -ULIBRARY -o dummy_client.o dummy_client.c
else 
	$(CC) -c $(CFLAGS) -ULIBRARY -o input.o input.c
	$(CC) -c $(CFLAGS) -ULIBRARY -o dvblo_handler.o dvblo_handler.c
endif
endif
	$(CC) $(LDFLAGS) -o $@ $(MCLI_SOBJS) $(LDLIBS) -L. -lmcli

$(MCLI)-static: $(MCLI_OBJS) $(MCLI_SOBJS)
	$(CC) $(LDFLAGS) -static -static-libgcc -o $@ $(MCLI_OBJS) $(MCLI_SOBJS) $(LDLIBS) $(LIBRARY_PATH)/libxml2.a $(LIBRARY_PATH)/libz.a $(LIBRARY_PATH)/libm.a $(LIBRARY_PATH)/libpthread.a
ifndef DEBUG 
ifndef WIN32
	strip $(MCLI)-static
endif
endif

api_shm_test.o: api_shm_test.c
	$(CC) -c $(CFLAGS) -DUSE_SHM_API -o $@ $<
	
api_sock_test.o:api_sock_test.c
	$(CC) -c $(CFLAGS) -DUSE_SOCK_API -o $@ $<
	
$(MCLI)-shmtest: api_shm_test.o
	$(CC) $(LDFLAGS) -o $@ api_shm_test.o $(LDLIBS) -lrt

$(MCLI)-socktest: api_sock_test.o
	$(CC) $(LDFLAGS) -o $@ api_sock_test.o 
	
install: mcli
	install -p $< /usr/sbin/$<

install-lib: libmcli.la
	libtool --mode=install install $< /usr/local/lib/	

install-shared: mcli-shared
	libtool --mode=install install $< /usr/local/bin

depend: .dependencies
	#makedepend -Y -- $(CFLAGS)  -- *c >/dev/null 2>&1

clean:
	rm -f $(MCLI) $(MCLI)-* *.elf *.gdb *.o *.lo *.la *~ *.so *.a *.def *.dll *.dylib out.ts

mingw32:
	rm -rf mingw/*.c mingw/*.h mingw/win32
	cp *.c *.h mingw/
	mkdir mingw/win32
	cp -a win32/lib mingw/win32/
	cp -a win32/include mingw/win32/
	@echo "Created mingw directory - now ready to rumble... (call build.cmd)"
        
%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

%.lo: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
