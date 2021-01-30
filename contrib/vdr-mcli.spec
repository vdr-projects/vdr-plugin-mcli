%global pname   mcli
%global __provides_exclude_from ^%{vdr_plugindir}/.*\\.so.*$

#global fork_account pbiering
#global fork_branch  vdr-2.4.1

#global gitcommit e050aa1e1bb9563bd7120b2715ec1d7e3da98256
#global gitshortcommit #(c=#{gitcommit}; echo ${c:0:7})
#global gitdate 20210123

%define rel	2

Name:           vdr-%{pname}
Version:        0.9.5
%if 0%{?gitcommit:1}
Release:        %{rel}.git.%{gitshortcommit}.%{gitdate}%{?dist}
%else
%if 0%{?fork_account:1}
Release:        %{rel}.%{?fork_account:.fork.%fork_branch}%{?dist}
%else
Release:        %{rel}%{?dist}
%endif
%endif
Summary:        DVB multicast stream client for the NetCeiver hardware for VDR

License:        GPLv2+ and LGPLv2+
URL:            https://github.com/vdr-projects/vdr-plugin-mcli
%if 0%{?fork_branch:1}
Source0:        https://github.com/%{fork_account}/vdr-plugin-mcli/archive/%{fork_branch}/%{name}.tar.gz
%else
%if 0%{?gitcommit:1}
Source0:        https://github.com/vdr-projects/vdr-plugin-mcli/archive/%{gitcommit}/%{name}-%{gitshortcommit}.tar.gz
%else
Source0:        https://github.com/vdr-projects/vdr-plugin-mcli/archive/v%{version}/%{name}-%{version}.tar.gz
%endif
%endif
Source1:        %{name}.conf
Source2:	systemd-vdr.service.d-override.conf

BuildRequires:  gcc-c++
BuildRequires:  vdr-devel >= 2.3.9
BuildRequires:  zlib-devel
BuildRequires:  libxml2-devel
Requires:       vdr(abi)%{?_isa} = %{vdr_apiversion}

%description
Multicast client plugin to access DVB-streams produced by the
NetCeiver hardware for VDR
%if 0%{?fork_account:1}
Fork: %{fork_account} / Branch: %{fork_branch}
%else
%if 0%{?gitcommit:1}
git-commit: %{gitshortcommit} from %{gitdate}
%endif
%endif


%package devel
Summary:        C header files for DVB multicast stream client 
Requires:       vdr-devel

%description devel
C header files for multicast client plugin to access DVB-streams
produced by the NetCeiver hardware for VDR
%if 0%{?fork_account:1}
Fork: %{fork_account} / Branch: %{fork_branch}
%endif


%prep
%if 0%{?fork_account:1}
%setup -q -n vdr-plugin-%{pname}-%{fork_branch}
%else
%if 0%{?gitcommit:1}
%setup -q -n vdr-plugin-%{pname}-%{gitcommit}
%else
%setup -q -n vdr-plugin-%{pname}-%{version}
%endif
%endif


%build
# enforce non-parallel build
%define _smp_mflags -j1

%make_build AUTOCONFIG=0

cd mcast/tool
%make_build AUTOCONFIG=0


%install
%make_install

pushd mcast/tool
mkdir -p $RPM_BUILD_ROOT/usr/sbin/
%make_install
popd

# plugin cofig file
if [ -e %{SOURCE1} ]; then
	# use from SOURCES / local directory
	echo "NOTICE: take default config file from SOURCES"
	install -Dpm 644 %{SOURCE1} $RPM_BUILD_ROOT%{_sysconfdir}/sysconfig/vdr-plugins.d/%{pname}.conf
elif [ -e contrib/$(basename "%{SOURCE1}") ]; then
	# use from inside tgz (rpmbuild -tb build)
	echo "NOTICE: take default config file from package"
	install -Dpm 644 contrib/$(basename "%{SOURCE1}") $RPM_BUILD_ROOT%{_sysconfdir}/sysconfig/vdr-plugins.d/%{pname}.conf
else
	echo "ERROR: can't find default config file %{SOURCE1}"
	exit 1
fi

# vdr/systemd/override.conf
if [ -e %{SOURCE2} ]; then
	# use from SOURCES / local directory
	echo "NOTICE: take vdr/systemd/override.conf from SOURCES"
	install -Dpm 644 %{SOURCE2} $RPM_BUILD_ROOT%{_sysconfdir}/systemd/system/vdr.service.d/override.conf
elif [ -e contrib/$(basename "%{SOURCE2}") ]; then
	# use from inside tgz (rpmbuild -tb build)
	echo "NOTICE: take vdr/systemd/override.conf from package"
	install -Dpm 644 contrib/$(basename "%{SOURCE2}") $RPM_BUILD_ROOT%{_sysconfdir}/systemd/system/vdr.service.d/override.conf
else
	echo "ERROR: can't find vdr/systemd/override.conf %{SOURCE2}"
	exit 1
fi

%find_lang %{name} --all-name --with-man

mkdir -p $RPM_BUILD_ROOT/usr/include/vdr/
install -Dpm 644 mcliheaders.h $RPM_BUILD_ROOT%{_includedir}/vdr


%files -f %{name}.lang
%license COPYING
%doc HISTORY README
%config(noreplace) %{_sysconfdir}/sysconfig/vdr-plugins.d/*.conf
%config(noreplace) %{_sysconfdir}/systemd/system/vdr.service.d/override.conf
%{vdr_plugindir}/libvdr-*.so.%{vdr_apiversion}
%{_sbindir}/*

%files devel
%{_includedir}/vdr/*.h


%changelog
* Sat Jan 30 2021 Peter Bieringer <pb@bieringer.de> - 0.9.5-2
- Include systemd/system/vdr.service.d/override.conf

* Tue Jan 26 2021 Peter Bieringer <pb@bieringer.de> - 0.9.5-1
- Update to new release
- Use default config file from package in case of rpmbuild -tX

* Sun Dec 20 2020 Peter Bieringer <pb@bieringer.de> - 0.9.4-2
- Create subpackage devel

* Sun Dec 20 2020 Peter Bieringer <pb@bieringer.de> - 0.9.4-1
- First build
