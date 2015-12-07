#!/bin/bash

# A script used to install FreeRDP-WebConnect's prerequisites based on the current Distro
# expects to be invoked with root privileges, either by way of sudo or as the root user itself

if [[ $EUID -ne 0 ]]; then
   echo "In order to install the dependencies of this package, the script must be run as root (or with sudo)"
   exit 2
fi

# script accepts a single parameter: -y . if specified, it will not ask for permission before removing/installing any packages.
# this is currently used only with CentOS where there are some outdated packages in the repository

if [[ $1 == '-y' ]]; then
	force_all=1
else
	force_all=0
fi

#Get distro (snipper take from alsa-info.sh)
DISTRO=`grep -ihs "buntu\|SUSE\|Fedora\|Debian\|CentOS\|Red Hat Enterprise Linux Server" /etc/{issue,*release,*version}`
case $DISTRO in
	*buntu*14*)
		echo 'Ubuntu 14.04 detected. Installing required packages...'
		apt-get update
		apt-get install -y python-software-properties
		echo | add-apt-repository ppa:ubuntu-toolchain-r/test
		apt-get update
		apt-get install -y build-essential g++-4.8 libxml++2.6-dev libssl-dev \
		libboost-all-dev libpng-dev libdwarf-dev subversion subversion-tools \
		autotools-dev autoconf libtool cmake
		# replace old gcc/g++ with new one
		rm /usr/bin/g++
		ln -s /usr/bin/g++-4.8 /usr/bin/g++
		rm /usr/bin/gcc
		ln -s /usr/bin/gcc-4.8 /usr/bin/gcc
		# installing things for FreeRDP
		apt-get -y install build-essential git-core cmake libssl-dev libx11-dev libxext-dev libxinerama-dev \
		  libxcursor-dev libxdamage-dev libxv-dev libxkbfile-dev libasound2-dev libcups2-dev libxml2 libxml2-dev \
		  libxrandr-dev libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev libxi-dev libavutil-dev \
		  libavcodec-dev libxtst-dev libgtk-3-dev libgcrypt11-dev libssh-dev libpulse-dev \
		  libvte-2.90-dev libxkbfile-dev libfreerdp-dev libtelepathy-glib-dev libjpeg-dev \
		  libgnutls-dev libgnome-keyring-dev libavahi-ui-gtk3-dev libvncserver-dev \
		  libappindicator3-dev intltool

		apt-get -y --purge remove freerdp-x11 \
		  remmina remmina-common remmina-plugin-rdp remmina-plugin-vnc remmina-plugin-gnome \
		  remmina-plugin-nx remmina-plugin-telepathy remmina-plugin-xdmcp
		;;
	Fedora*)
		echo 'Fedora detected.Installing required packages...'
		yum install -y gcc-c++ autoconf automake libtool cmake svn svn2cl \
		openssl-devel boost-devel libpng-devel elfutils-devel
		;;
	*SUSE*)
		echo 'SUSE detected.Installing required packages...'
		zypper --non-interactive install gcc-c++ make cmake openssl-devel zlib-devel boost-devel libpng-devel
		;;
	Red\sHat\sEnterprise\sLinux\sServer*7*|CentOS*7*)
		echo 'CentOS detected. Installing required packages...'
		yum install -y gcc-c++ svn subversion-svn2cl openssl-devel \
		libpng-devel elfutils-devel glib2-devel
		# check if cmake is already installed
		yum list installed | grep -i cmake
		if [ $? -eq 0 ]; then
			if [[ $force_all == 0 ]]; then
				echo -n 'cmake is already installed. CentOS 6.4 uses an old version of cmake. Do you wish to remove the current version and get a newer one? [y/N]'
				read response_remove
				echo ''
				if [[ $response_remove == 'y' ]]; then
					yum -y remove cmake
					response_install='y'
				else
					echo -n 'Do you still wish to install the newer version? [y/N]'
					read response_install
					echo ''
				fi
			else
				yum -y remove cmake
				response_install='y'
			fi
		else
			response_install='y'
		fi
		
		if [[ $response_install == 'y' || $force_all == 1 ]]; then
			# Installing CMake from sources
			if [ ! -d "cmake-3.0.0" ]; then
				wget http://www.cmake.org/files/v3.0/cmake-3.0.0.tar.gz
				tar zxvf cmake-3.0.0.tar.gz
			fi
			pushd .
			cd cmake-3.0.0
			./configure
			make install
			popd
		fi
		
		# check for autoconf
		yum list installed | grep -i autoconf
		if [ $? -eq 0 ]; then
			if [[ $force_all == 0 ]]; then
				echo -n 'autoconf is already installed. CentOS 6.4 uses an old version of autoconf. Do you wish to remove the current version and get a newer one? [y/N]'
				read response_remove
				echo ''
				if [[ $response_remove == 'y' ]]; then
					yum -y remove autoconf
					response_install='y'
				else
					echo -n 'Do you still wish to install the newer version? [y/N]'
					read response_install
				fi
			else
				yum -y remove autoconf
				response_install='y'
			fi
		else
			response_install='y'
		fi
		
		if [[ $response_install == 'y' || $force_all == 1 ]]; then
			rpm -ivh wsgate/deps/rpms/autoconf-2.69-12.2.noarch.rpm
		fi
		
		# autoconf is a prereq of automake
		yum install -y automake libtool

		# Install CERN's devtoolset-2

		wget -O /etc/yum.repos.d/slc6-devtoolset.repo http://linuxsoft.cern.ch/cern/devtoolset/slc6-devtoolset.repo
		rpm --import http://www.scientificlinux.org/documentation/gpg/RPM-GPG-KEY-cern
		yum install -y devtoolset-2-gcc-c++ devtoolset-2-libstdc++-devel devtoolset-2-binutils
		export PATH=/opt/rh/devtoolset-2/root/usr/bin:$PATH

		# Download install an updated Boost version
		if [ ! -d "boost_1_55_0" ]; then
			wget http://downloads.sourceforge.net/project/boost/boost/1.55.0/boost_1_55_0.tar.gz
			tar zxvf boost_1_55_0.tar.gz
		fi

		rpm -q boost-devel > /dev/null
		if [ $? -eq 0 ]; then
			# TODO: ask to uninstall boost-devel if present
			rpm -e boost-devel
		fi

		pushd .
		cd boost_1_55_0
		./bootstrap.sh
		./b2 install
		popd

		;;
	CentOS*5*)
		echo 'CentOS 5 not yet supported! Please install prereqs by hand:'
		echo 'git, svn-devel, autotools, gcc, g++, boost, openssl-devel and libpng-devel. For tracing to work, libdwarf is also required'
		exit 1
		;;
	*)
		echo 'Distro not found! You shall need git, svn-devel, autotools, gcc, g++, boost, openssl-devel and libpng-devel to successfully build this package.'
		echo 'For tracing to work, libdwarf is also required.'
		exit 1
		;;
esac
exit 0
