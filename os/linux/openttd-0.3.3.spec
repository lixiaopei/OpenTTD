%define name openttd 
%define version 0.3.3
%define release 1mdk

Name: %{name} 
Summary: An open source clone of the Microprose game "Transport Tycoon Deluxe"
Version: %{version} 
Release: %{release} 
Source0: %{name}-%{version}.tar.bz2
Source10: %{name}.64.png
Patch0: %{name}-%{version}-patch.diff
URL: http://www.openttd.org
Group: Games/Strategy
Packager: Dominik Scherer <>
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot 
License: GPL
BuildRequires: libSDL1.2-devel >= 1.2.7
BuildRequires: libpng3-devel >= 1.2.5 
BuildRequires: zlib1-devel >= 1.2.1

%description
An enhanced open source clone of the Microprose game "Transport Tycoon Deluxe".
You require the data files of the original Transport Tycoon Deluxe
for Windows to play the game. You have to MANUALLY copy them to the
game data directory!

%prep 
rm -rf $RPM_BUILD_ROOT 
%setup
%patch -p1

%build
make BINARY_DIR=%{_gamesbindir}/openttd/ INSTALL_DIR=%{_gamesdatadir}/openttd/ GAME_DATA_DIR=%{_gamesdatadir}/openttd/ USE_HOMEDIR=1 PERSONAL_DIR=.openttd

%install
mkdir -p $RPM_BUILD_ROOT%{_gamesbindir}/openttd
mkdir -p $RPM_BUILD_ROOT%{_gamesdatadir}/openttd/lang

cp ./ttd $RPM_BUILD_ROOT%{_gamesbindir}/openttd/ttd
cp -r ./lang/*.lng $RPM_BUILD_ROOT%{_gamesdatadir}/openttd/lang/
cp -r ./data $RPM_BUILD_ROOT%{_gamesdatadir}/openttd/

# icon
install -m644 %{SOURCE10} -D $RPM_BUILD_ROOT%{_miconsdir}/%{name}.png
install -m644 %{SOURCE10} -D $RPM_BUILD_ROOT%{_iconsdir}/%{name}.png
install -m644 %{SOURCE10} -D $RPM_BUILD_ROOT%{_liconsdir}/%{name}.png

# menu entry
mkdir -p $RPM_BUILD_ROOT/%{_menudir}
cat << EOF > $RPM_BUILD_ROOT/%{_menudir}/%{name}
?package(%{name}):command="%{_gamesbindir}/openttd/ttd" icon="%{name}.png" \
  needs="X11" section="Amusement/Strategy" title="OpenTTD" \
  longtitle="%{Summary}"
EOF

%clean 
rm -rf $RPM_BUILD_ROOT 

%post
%{update_menus}

%postun
%{clean_menus}

%files 
%defattr(-,root,root,0755) 
%{_gamesbindir}/openttd/ttd

%{_gamesdatadir}/openttd/lang/czech.lng
%{_gamesdatadir}/openttd/lang/danish.lng
%{_gamesdatadir}/openttd/lang/english.lng
%{_gamesdatadir}/openttd/lang/french.lng
%{_gamesdatadir}/openttd/lang/galician.lng
%{_gamesdatadir}/openttd/lang/german.lng
%{_gamesdatadir}/openttd/lang/hungarian.lng
%{_gamesdatadir}/openttd/lang/italian.lng
%{_gamesdatadir}/openttd/lang/norwegian.lng
%{_gamesdatadir}/openttd/lang/polish.lng
%{_gamesdatadir}/openttd/lang/slovak.lng
%{_gamesdatadir}/openttd/lang/swedish.lng

%{_gamesdatadir}/openttd/data/canalsw.grf
%{_gamesdatadir}/openttd/data/openttd.grf
%{_gamesdatadir}/openttd/data/opntitle.dat
%{_gamesdatadir}/openttd/data/signalsw.grf

%{_menudir}/%{name}
%{_iconsdir}/*.png
%{_miconsdir}/*.png
%{_liconsdir}/*.png

%doc changelog.txt readme.txt COPYING

%changelog 
* Wed Jul 31 2004 Dominik Scherer <> 0.3.3-1mdk
- Initial release
