#
# spec file for package libappcontroller
#

%define _prefix /opt/RP
%define __cmake cmake

%if 0%{?fedora_version}
%global debug_package %{nil}
%endif

Name:           rp-libappcontroller
# WARNING {name} is not used for tar file name in source nor for setup
#         Check hard coded values required to match git directory naming
Version:        2.0
Release:        0
License:        Apache-2.0
Summary:        AGL libappcontroller
Group:          Development/Libraries/C and C++
Url:            https://gerrit.automotivelinux.org/gerrit/#/admin/projects/src/libappcontroller
Source:         libappcontroller-%{version}.tar.gz
BuildRequires:  cmake
BuildRequires:  curl
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig(json-c)
BuildRequires:  pkgconfig(afb-daemon)
BuildRequires:  pkgconfig(lua) >= 5.3
BuildRequires:  pkgconfig(afb-helpers)
BuildRequires:  pkgconfig(libsystemd) >= 222

BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
libappcontroller provides helpful functions to be used in a binding for the
Application Framework Binder

%package devel
Group:          Development/Libraries/C and C++
Requires:       %{name} = %{version}
Provides:       pkgconfig(%{name}) = %{version}
Summary:        AGL libappcontroller-devel
%description devel
libappcontroller provides helpful functions to be used in a binding for the
Application Framework Binder

%prep
%setup -q -n libappcontroller-%{version}

%build
export PKG_CONFIG_PATH=%{_libdir}/pkgconfig
[ ! -d build ] && mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/opt/AGL -DCMAKE_BUILD_TYPE=DEBUG -DVERSION=%{version} ..
%__make %{?_smp_mflags}

%install
[ -d build ] && cd build
%make_install

%files

%files devel
%defattr(-,root,root)
%dir %{_includedir}
%dir %{_libdir}
%{_includedir}/*
%{_libdir}/*.a
%{_libdir}/pkgconfig/*.pc

%changelog
* Fri Dec 28 2018 Romain Forlot
- initial creation
