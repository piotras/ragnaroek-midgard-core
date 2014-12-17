%define major_version 8.10.0

%if 0%{?suse_version}
%define devel_requires glib2-devel >= 2.12, libxml2-devel >= 2.6, dbus-1-devel, dbus-1-glib-devel, mysql-devel >= 5.0, libopenssl-devel
%else
%define devel_requires glib2-devel >= 2.12, libxml2-devel >= 2.6, dbus-devel, dbus-glib-devel, mysql-devel >= 5.0, openssl-devel
%endif

Name:           midgard-core
Version:        %{major_version}
Release:        OBS
Summary:        Midgard core library and tools

Group:          System Environment/Base
License:        LGPLv2+
URL:            http://www.midgard-project.org/
Source0:        %{url}download/%{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  pkgconfig >= 0.9.0
BuildRequires:  %{devel_requires}
BuildRequires:  flex
BuildRequires:  pam-devel

Requires:       glib2 >= 2.12, libxml2 >= 2.6
Requires(post): /bin/ls, /bin/grep

%description                
Midgard is a persistent storage framework built for the replicated
world. It enables developers build applications that have their data in
sync between the desktop, mobile devices and web services. It also
allows for easy sharing of data between users.

Midgard does this all by building on top of technologies like GLib and 
D-Bus. It provides developers with object-oriented programming 
interfaces for C and PHP (next version: Python).

This package provides the core C library and tools of the Midgard 
framework. The library allows Midgard applications to access the MySQL 
database (next version: database-independence). The library also does 
user authentication and privilege handling.


%package        devel
Summary:        Development files for %{name}
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}
Requires:       %{devel_requires}

%description devel
The %{name}-devel package contains libraries and header files for 
developing applications that use %{name}.


%prep
%setup -q


%build
%configure --disable-static
make %{?_smp_mflags}


%install
%if 0%{?suse_version} == 0
rm -rf $RPM_BUILD_ROOT
mkdir -p $(dirname $RPM_BUILD_ROOT)
mkdir $RPM_BUILD_ROOT
%endif
make install DESTDIR=$RPM_BUILD_ROOT
find $RPM_BUILD_ROOT -name '*.la' -exec rm -f {} ';'


%clean
rm -rf $RPM_BUILD_ROOT


%post
/sbin/ldconfig
/bin/ls -1 %{_sysconfdir}/midgard/conf.d/ | /bin/grep -v ^midgard\.conf\.example$ | while read file; do
    %{_bindir}/midgard-schema "$file" > /dev/null 2>&1
done
exit 0

%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%doc COPYING
%{_libdir}/*.so.*
%dir %{_sysconfdir}/midgard
%dir %{_sysconfdir}/midgard/conf.d
%config(noreplace,missingok) %{_sysconfdir}/midgard/conf.d/*
%{_bindir}/*
%{_mandir}/man1/*
%dir %{_datadir}/midgard
%{_datadir}/midgard/*

%files devel
%defattr(-,root,root,-)
%dir %{_includedir}/midgard
%{_includedir}/midgard/*
%{_libdir}/*.so
%{_libdir}/pkgconfig/*


%changelog
* Thu Sep 17 2009 Jarkko Ala-Louvesniemi <jval@puv.fi> 8.09.5-8.1
- Added missingok for the example configuration file(s)

* Thu Jul 16 2009 Jarkko Ala-Louvesniemi <jval@puv.fi> 8.09.5
- Initial OBS package based on the Fedora spec.
