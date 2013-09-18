OBJS=main.o parse.o mclilink.o siparser.o crc32.o clist.o stream.o thread.o misc.o streamer.o igmp.o iface.o
EXE=netcv2dvbip.exe
CC:=gcc
CXX=g++
CFLAGS:= -O2 -DCLIENT -DSTATICLIB -Wall -I../../common/win32/include -I../../common -I../../client
LDFLAGS:= -s -L../../common/win32/lib
LDLIBS:= -lmingwex -lmcli -liphlpapi -lpthreadGC2 -lws2_32 -lxml2 -lz

all: $(EXE)

$(EXE): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: ../%.c
	$(CXX) -c $(CFLAGS) -o $@ $<

clean:
	@-del  $(EXE) *.o *.la *~

