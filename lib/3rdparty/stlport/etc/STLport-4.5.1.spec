%define MAKEFILE gcc-linux.mak

Summary: Complete C++ standard library
Name: STLport
Version: 4.5.1
Release: 1
Copyright: free (see license), see /usr/share/doc/%{name}-%{version}/license.html
URL: http://www.stlport.org/
Packager: Levente Farkas <lfarkas@mindmaker.hu>
Group: System Environment/Languages
Icon: stlport_powered_white.gif
Source0: http://www.stlport.org/archive/%{name}-%{version}.tar.gz
Patch0: STLport-rename.patch
#Patch1: STLport-rules.patch
#Patch2: STLport-install-dir.patch
Buildroot: %{_tmppath}/%{name}-%{version}-%(id -u -n)

%description
STLport is a multiplatform STL implementation based on SGI STL.
This package contains the runtime library for STLport.

%package -n STLport-devel
Summary: Complete C++ standard library header files and libraries
Group: Development/Libraries
Requires: STLport = %{version}

%description -n STLport-devel
STLport is a multiplatform STL implementation based on SGI STL. Complete   
C++ standard library, including <complex> and SGI STL iostreams. If you
would like to use your code with STLport add
"-nostdinc++ -I/usr/include/stlport" when compile and -lstlport_gcc when
link (eg: gcc -nostdinc++ -I/usr/include/stlport x.cc -lstlport_gcc).

%prep
%setup
%patch0 -p1
#%patch1 -p1
#%patch2 -p1

%build
cd src
make -f %{MAKEFILE} INSTALLDIR=$RPM_BUILD_ROOT/usr clean all

%install
rm -rf $RPM_BUILD_ROOT
cd src
make -f %{MAKEFILE} INSTALLDIR=$RPM_BUILD_ROOT/usr install
cd $RPM_BUILD_ROOT/usr/include/stlport
ln -s . ext

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%post -n STLport-devel
/sbin/ldconfig

%postun -n STLport-devel
/sbin/ldconfig

%files
%defattr(-,root,root)
%doc doc/license.html
/usr/lib/libstlport_gcc.so
#/usr/lib/libstlport_gcc.so.%{version}
/usr/lib/libstlport_gcc.so.4.5

%files -n STLport-devel
%defattr(-,root,root)
%doc INSTALL README doc etc test
/usr/lib/libstlport_gcc*.a
/usr/lib/libstlport_gcc_*debug.so*
/usr/include/*

%changelog
* Mon Dec 10 2001 Levente Farkas <lfarkas@mindmaker.hu>
- upgrade to 4.5.1

* Fri Nov 16 2001 Levente Farkas <lfarkas@mindmaker.hu>
- merge with Harold's changes

* Thu Nov 15 2001 <stlport@lanceerplaats.nl>
- rebuild for RedHat 7.2, spec file fixes.

* Tue Oct  2 2001 Levente Farkas <lfarkas@mindmaker.hu>
- upgrade to 4.5

* Thu Oct 26 2000 Levente Farkas <lfarkas@mindmaker.hu>
- upgrade to 4.1-b3

* Thu Jul 17 2000 Levente Farkas <lfarkas@mindmaker.hu>
- initial release use STLport-4.0

