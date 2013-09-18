#
# Makefile for a Video Disk Recorder plugin
#

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
# IMPORTANT: the presence of this macro is important for the Make.config
# file. So it must be defined, even if it is not used here!
#
#MCLI_SHARED=1
PLUGIN = mcli
APPLE_DARWIN = $(shell gcc -dumpmachine | grep -q 'apple-darwin' && echo "1" || echo "0")

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The C++ compiler and options:

CXX      ?= g++
ifeq ($(APPLE_DARWIN), 1)
CXXFLAGS ?= -fPIC -pg -O2 -Wall -Woverloaded-virtual -Wno-parentheses -fno-common -bundle -flat_namespace -undefined suppress
else
CXXFLAGS ?= -fPIC -pg -O2 -Wall -Woverloaded-virtual -Wno-parentheses
endif

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Make sure that necessary options are included:

-include $(VDRDIR)/Make.global

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config

### The version number of VDR's plugin API (taken from VDR's "config.h"):

APIVERSION = $(shell sed -ne '/define APIVERSION/s/^.*"\(.*\)".*$$/\1/p' $(VDRDIR)/config.h)

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### Includes and Defines (add further entries here):

ifdef RBMINI
  XML_INC := -I/usr/arm-linux-gnueabi/include/libxml2
  XML_LIB := -lxml2
else
  XML_INC := `xml2-config --cflags`
  XML_LIB := `xml2-config --libs`
endif


ifdef MCLI_SHARED
  LIBS = -lmcli -Lmcast/client $(XML_LIB)
else
  LIBS = mcast/client/libmcli.a $(XML_LIB)
endif

INCLUDES += -I$(VDRDIR)/include -I. $(XML_INC)


ifeq ($(APPLE_DARWIN), 1)
INCLUDES += -I/sw/include -Imcast/common/darwin/include/
DEFINES += -DAPPLE
ifdef MCLI_SHARED
DEFINES += -Imcast/common/darwin/include/
endif
endif

ifdef REELVDR
  DEFINES += -DREELVDR 
endif

DEFINES += -D_GNU_SOURCE -DPLUGIN_NAME_I18N='"$(PLUGIN)"'
# -DDEVICE_ATTRIBUTES

### The object files (add further files here):

OBJS = $(PLUGIN).o cam_menu.o device.o filter.o packetbuffer.o

### The main target:

plug: libmcli.so libvdr-$(PLUGIN).so

all: libmcli.so libvdr-$(PLUGIN).so i18n

libmcli.so:
	$(MAKE) -C mcast/client/


### Implicit rules:

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

### Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

### Internationalization (I18N):

PODIR     = po
LOCALEDIR = $(VDRDIR)/locale
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmsgs  = $(addprefix $(LOCALEDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

%.mo: %.po
	msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.c)
	xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP --msgid-bugs-address='<see README>' -o $@ $^

%.po: $(I18Npot)
	msgmerge -U --no-wrap --no-location --backup=none -q $@ $<
	@touch $@

$(I18Nmsgs): $(LOCALEDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	@mkdir -p $(dir $@)
	cp $< $@

.PHONY: i18n
i18n: $(I18Nmsgs) $(I18Npot)

i18n-dist: $(I18Nmsgs)

### Targets:

libvdr-$(PLUGIN).so: $(OBJS)
ifeq ($(APPLE_DARWIN), 1)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIBS) -o $@
	@cp $@ $(LIBDIR)/$@.$(APIVERSION)
else
	$(CXX) $(CXXFLAGS) -shared $(OBJS) $(LIBS) -o $@
	@cp --remove-destination $@ $(LIBDIR)/$@.$(APIVERSION)
endif

install:
ifdef MCLI_SHARED
	install -m 755 -D mcast/client/libmcli.so /usr/lib
endif
	install -m 755 -D libvdr-$(PLUGIN).so $(PLUGINLIBDIR)/libvdr-$(PLUGIN).so.$(APIVERSION)


dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(OBJS) $(DEPFILE) *.so *.tgz core* *~ $(PODIR)/*.mo $(PODIR)/*.pot $(LIBDIR)/libvdr-$(PLUGIN).so.* mcast/client/*.o mcast/client/*.so mcast/client/*.a
