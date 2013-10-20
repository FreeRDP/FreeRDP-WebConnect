#!/bin/bash

# A script used to install FreeRDP-WebConnect's prerequisites based on the current Distro
# expects to be invoked with root privileges, either by way of sudo or as the root user itself

if [[ $EUID -ne 0 ]]; then
   echo "In order to install the dependencies of this package, the script must be run as root (or with sudo)"
   exit 2
fi

#Get distro (snipper take from alsa-info.sh)
DISTRO=`grep -ihs "buntu\|SUSE\|Fedora\|Debian\|CentOS" /etc/{issue,*release,*version}`
case $DISTRO in
	*buntu*)
		echo 'Ubuntu distro detected. Installing required packages...'
		;;
	Fedora*19*)
		echo 'Fedora 19 detected.Installing required packages...'
		yum install -y gcc-c++ autoconf automake libtool cmake svn svn2cl \
		openssl-devel boost-devel libpng-devel elfutils-devel
		;;
	SUSE*)
		echo 'SUSE detected. Installing required packages...'
		;;
	CentOS*)
		echo 'CentOS detected. Installing required packages...'
		yum install -y svn subversion-svn2cl openssl-devel boost-devel libpng-devel elfutils-devel
		yum groupinstall -y "Development tools"
		echo 'Getting cmake 2.8.12 (CentOS has old cmake version)'
		wget http://www.cmake.org/files/v2.8/cmake-2.8.12.tar.gz && tar -xzvf cmake-2.8.12.tar.gz && cd cmake-2.8.12 && ./bootstrap --prefix=/usr && make && make install
		cd ..
		rm -rf cmake-2.8.12 && rm -f cmake-2.8.12.tar.gz
		;;
		*)
		echo 'Distro not found! You shall need git, svn-devel, autotools, gcc, g++, boost, openssl-devel and libpng-devel to successfully build this package.'
		echo 'For tracing to work, libdwarf is also required.'
		exit 1
		;;
esac
exit 0