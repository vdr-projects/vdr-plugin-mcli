#
# Makefile for a Video Disk Recorder plugin
#

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.

PLUGIN = mcli

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).c | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The directory environment:

# Use package data if installed...otherwise assume we're under the VDR source directory:
PKGCFG = $(if $(VDRDIR),$(shell pkg-config --variable=$(1) $(VDRDIR)/vdr.pc),$(shell pkg-config --variable=$(1) vdr || pkg-config --variable=$(1) ../../../vdr.pc))
LIBDIR = $(call PKGCFG,libdir)
LOCDIR = $(call PKGCFG,locdir)
PLGCFG = $(call PKGCFG,plgcfg)
#
TMPDIR ?= /tmp

### The compiler options:

export CFLAGS   = $(call PKGCFG,cflags) -fPIC -Wall
export CXXFLAGS = $(call PKGCFG,cxxflags) -fPIC -Wall

### The version number of VDR's plugin API:

APIVERSION = $(call PKGCFG,apiversion)

### Allow user defined options to overwrite defaults:

-include $(PLGCFG)

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### The name of the shared object file:

SOFILE = libvdr-$(PLUGIN).so

### Includes and Defines (add further entries here):

.PHONY: i18n all clean

XML_INC ?= $(shell xml2-config --cflags)
XML_LIB ?= $(shell xml2-config --libs)

ifdef MCLI_SHARED
  LIBS = -Lmcast/client -lmcli $(XML_LIB)
else
  LIBS = mcast/client/libmcli.a $(XML_LIB)
endif

INCLUDES += -I$(VDRDIR)/include -I. $(XML_INC)

DEFINES += -D_GNU_SOURCE -DPLUGIN_NAME_I18N='"$(PLUGIN)"'
# -DDEVICE_ATTRIBUTES

### The object files (add further files here):

OBJS = $(PLUGIN).o cam_menu.o device.o filter.o packetbuffer.o

### The main target:

all:  lib plugin tools i18n


plugin: i18n
	$(MAKE) XML_INC="$(XML_INC)" XML_LIB="$(XML_LIB)" libvdr-$(PLUGIN).so

tools: lib
	 $(MAKE) XML_INC="$(XML_INC)" XML_LIB="$(XML_LIB)" -C mcast/client/ mcli
	 $(MAKE) XML_INC="$(XML_INC)" XML_LIB="$(XML_LIB)" -C mcast/tool/ all

lib:
	$(MAKE) XML_INC="$(XML_INC)" XML_LIB="$(XML_LIB)" libmcli.so

libmcli.a libmcli.so:
	$(MAKE) XML_INC="$(XML_INC)" XML_LIB="$(XML_LIB)" -C mcast/client/ libmcli

### Implicit rules:

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

### Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(CXXFLAGS) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

### Internationalization (I18N):

PODIR     = po
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmsgs  = $(addprefix $(DESTDIR)$(LOCDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

%.mo: %.po
	msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.c)
	xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP --msgid-bugs-address='<see README>' -o $@ $^

%.po: $(I18Npot)
	msgmerge -U --no-wrap --no-location --backup=none -q $@ $<
	@touch $@

$(I18Nmsgs): $(DESTDIR)$(LOCDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	@mkdir -p $(dir $@)
	cp $< $@

i18n: $(I18Npot)

i18n-dist: $(I18Nmsgs)

### Targets:
$(SOFILE): $(OBJS) libmcli.a
ifeq ($(APPLE_DARWIN), 1)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIBS) -o $@
else
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $(OBJS) $(LIBS) -o $@
endif

install-lib: $(SOFILE)
	install -D $^ $(DESTDIR)$(LIBDIR)/$^.$(APIVERSION)

install: install-lib i18n-dist

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(OBJS) $(DEPFILE) *.so *.tgz core* *~  po/*.mo po/*.pot
	$(MAKE) -C mcast/client/ clean
	$(MAKE) -C mcast/netcv2dvbip/ clean
	$(MAKE) -C mcast/tool/ clean

