%define __spec_install_post %{nil}
%define debug_package %{nil}
%define __os_install_post ${_dbpath}/brp-compress

Summary: Efficient statsd replacement
Name: brubeck
Version: 1.0.0
Release: 1
License: none
SOURCE0: %{name}-%{version}.tar.gz
BuildRoot: $PWD/BUILDROOT
URL: https://github.com/deanbittner/brubeck

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
${summary}

%prep
%setup -q

%build
# empty section

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}
#in builddir
cp -a * %{buildroot}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%config(noreplace) /etc/brubeck/config.json
/usr/local/sbin/brubeck
/etc/init.d/brubeck

%changelog
* Mon Feb 13 2017 Dean Bittner <dean@run-withit.com>
- first rpm release of v1
