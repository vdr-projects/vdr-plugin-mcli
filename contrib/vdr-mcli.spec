## Build instructions
# rpmbuild -bb --undefine=_disable_source_fetch vdr-mcli.spec

## Build of a specific branch in upstream repository
# rpmbuild -bb --undefine=_disable_source_fetch -D "branch <branch>" vdr-mcli.spec

## Build of a specific branch in forked repository
# rpmbuild -bb --undefine=_disable_source_fetch -D "account <account>" -D "branch <branch>" vdr-mcli.spec

%global pname   mcli
%global __provides_exclude_from ^%{vdr_plugindir}/.*\\.so.*$

# override account
%if 0%{?account:1}
%global fork_account %{account}
%if 0%{?branch:1}
%else
%global branch master
%endif
%else
%global account vdr-projects
%endif

%define rel	5

Name:           vdr-%{pname}
Summary:        DVB multicast stream client for the NetCeiver hardware for VDR
Version:        1.0.0
Release:        %{rel}%{?fork_account:.fork.%fork_account}%{?branch:.branch.%(echo "%{branch}" | sed 's/-//g')}%{?dist}

License:        GPLv2+
URL:            https://github.com/vdr-projects/vdr-plugin-mcli

%if 0%{?branch:1} || 0%{?fork_account:1}
Source0:        https://github.com/%{account}/vdr-plugin-mcli/archive/%{branch}/%{name}.tar.gz
%else
Source0:        https://github.com/vdr-projects/vdr-plugin-mcli/archive/v%{version}/vdr-plugin-%{pname}-%{version}.tar.gz
%endif

%define		file_plugin_config		%{name}.conf
%define		file_systemd_override_config	systemd-vdr.service.d-override.conf

BuildRequires:  gcc-c++
BuildRequires:  vdr-devel >= 2.3.9
BuildRequires:  zlib-devel
BuildRequires:  libnetceiver-devel >= 0.0.6

Requires:       vdr(abi)%{?_isa} = %{vdr_apiversion}
Requires:       libnetceiver >= 0.0.6

Conflicts:      vdr-mcli-devel


%description
Multicast client plugin to access DVB-streams produced by the
 NetCeiver hardware for VDR
Contains also systemd/override config for vdr to allow vdr to
 open raw sockets:
  %{_sysconfdir}/systemd/system/vdr.service.d/vdr-mcli.conf


%prep
%if 0%{?branch:1}
%setup -q -n vdr-plugin-%{pname}-%{branch}
%else
%setup -q -n vdr-plugin-%{pname}-%{version}
%endif


%build
# enforce non-parallel build
%define _smp_mflags -j1

%make_build AUTOCONFIG=0


%install
%make_install

# plugin config file
install -Dpm 644 contrib/%{file_plugin_config} $RPM_BUILD_ROOT%{_sysconfdir}/sysconfig/vdr-plugins.d/%{pname}.conf

# vdr/systemd/override.conf
install -Dpm 644 contrib/%{file_systemd_override_config} $RPM_BUILD_ROOT%{_sysconfdir}/systemd/system/vdr.service.d/vdr-mcli.conf


%find_lang %{name} --all-name --with-man


%files -f %{name}.lang
%license COPYING
%doc HISTORY README
%config(noreplace) %{_sysconfdir}/sysconfig/vdr-plugins.d/*.conf
%{_sysconfdir}/systemd/system/vdr.service.d/vdr-mcli.conf
%{vdr_plugindir}/libvdr-*.so.%{vdr_apiversion}


%post
systemctl daemon-reload


%changelog
* Wed May 31 2023 Peter Bieringer <pb@bieringer.de> - 1.0.0-5
- Update to new release which depends on libnetceiver
- Remove devel subpackage (now header files included in libnetceiver-devel)

* Mon Feb 07 2022 Peter Bieringer <pb@bieringer.de> - 0.9.7-4
- Update to new release

* Mon Feb 08 2021 Peter Bieringer <pb@bieringer.de> - 0.9.5-4
- Use only plugin/systemd config from contrib subdirectory

* Sun Jan 31 2021 Peter Bieringer <pb@bieringer.de> - 0.9.5-3
- Change name to systemd/system/vdr.service.d/vdr-mcli.conf
- Add on %post systemctl daemon-reload

* Sat Jan 30 2021 Peter Bieringer <pb@bieringer.de> - 0.9.5-2
- Include systemd/system/vdr.service.d/override.conf

* Tue Jan 26 2021 Peter Bieringer <pb@bieringer.de> - 0.9.5-1
- Update to new release
- Use default config file from package in case of rpmbuild -tX

* Sun Dec 20 2020 Peter Bieringer <pb@bieringer.de> - 0.9.4-2
- Create subpackage devel

* Sun Dec 20 2020 Peter Bieringer <pb@bieringer.de> - 0.9.4-1
- First build
