# Note that this is NOT a relocatable package
%define prefix          /usr
%define bindir          /usr/bin
%define mandir          /usr/share/man
%define tmppath         /tmp

Summary:    Grapically displays the amount of disks used for by each subdirectory.
Name:       xdiskusage
Version:    1.43
Release:    1
Source0:    http://xdiskusage.sourceforge.net/%{name}-%{version}.tgz
Copyright:  GPL
Group:      Applications/System
BuildRoot:  %{tmppath}/%{name}-%{version}-buildroot
URL:        http://xdiskusage.sourceforge.net/
BuildRequires:   fltk-devel
BuildRequires:   Mesa

%description
xdiskusage is a user-friendly program to show you what is using 
up all your disk space. It is based on the design of xdu 
written by Phillip C. Dykstra. Changes have been made so it runs 
"du" for you, and can display the free space left on the disk, 
and produce a PostScript version of the display.

%prep
%setup

%build
%configure
./configure --prefix=%prefix
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{bindir} $RPM_BUILD_ROOT/%{mandir}/man1
make prefix=$RPM_BUILD_ROOT/%{prefix} \
	mandir=$RPM_BUILD_ROOT/%{mandir} install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc README
%{bindir}/*
%{mandir}/man1/*

%changelog
* Thu May 3 2001 Dan Mueth <dan@eazel.com>
- First draft of a spec file. I'm not modifying the autoconf or make stuff
  so I had to do a couple goofy things in here.

