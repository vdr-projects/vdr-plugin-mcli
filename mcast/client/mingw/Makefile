PATH:=/MinGW/bin/:$(PATH)
CC:=gcc

#Comment this out to disable debugging output
DEBUG=1
#VERBOSE=1
#API_SOCK=1

DEFS=-DCLIENT -DLIBRARY -DWIN32 -D_REENTRANT -D_GNU_SOURCE

ifdef VERBOSE
DEFS:= $(DEFS) -DDEBUG
DEBUG=1
endif

CFLAGS:= $(DEFS) -Wall -Iwin32/include $(CFLAGS)
LDFLAGS:= -Lwin32/lib $(LDFLAGS)
LDLIBS:= -lwsock32 -liphlpapi -lpthreadGC2 -lxml2 -lzdll

ifdef DEBUG
LDFLAGS:= $(LDFLAGS) -g
CFLAGS:= $(CFLAGS) -g 
else
#CFLAGS:= $(CFLAGS) -Os
endif

MCLI = mcli
MCLI_OBJS = mld_common.o mld_client.o mld_reporter.o mcast.o recv_tv.o recv_ccpp.o tools.o tca_handler.o tra_handler.o satlists.o interfaces.o inet_pton.o inet_ntop.o inet_aton.o
MCLI_SOBJS := main.o dummy_client.o

all: lib$(MCLI)

lib$(MCLI): $(MCLI_OBJS) 
	$(CC) $(LDFLAGS) -shared -o $@.dll $(MCLI_OBJS) $(LDLIBS) -Wl,--out-implib,$@.a -Wl,--output-def,$@.def
#	$(AR) $(ARFLAGS) $@.a $(MCLI_OBJS)
	@copy /b $@.dll win32\\lib\\
	@copy /b $@.a win32\\lib\\
	@copy /b $@.def win32\\lib\\
	lib.exe /def:$@.def /machine:x86 /out:win32\\lib\\$@.lib
	@echo "You can find all libraries in directory win32\lib"

$(MCLI): $(MCLI_OBJS) $(MCLI_SOBJS)
	$(CC) $(LDFLAGS) -o $@ $(MCLI_OBJS) $(MCLI_SOBJS) $(LDLIBS) 

$(MCLI)-shared: lib$(MCLI)
	$(CC) -c $(CFLAGS) -ULIBRARY -o main.o main.c
	$(CC) -c $(CFLAGS) -ULIBRARY -o dummy_client.o dummy_client.c
	$(CC) $(LDFLAGS) -o $@ $(MCLI_SOBJS) $(LDLIBS) -lmcli

clean:
	@del  $(MCLI)*.exe lib$(MCLI).* *.lib *.o *.la *~

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

