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
DISTRO=`grep -ihs "buntu\|SUSE\|Fedora\|Debian\|CentOS" /etc/{issue,*release,*version}`
case $DISTRO in
	*buntu*)
		echo 'Ubuntu not yet supported! Please install prereqs by hand:'
		echo 'git, svn-devel, autotools, gcc, g++, boost, openssl-devel and libpng-devel. For tracing to work, libdwarf is also required'
		exit 1
		;;
	Fedora*)
		echo 'Fedora detected.Installing required packages...'
		yum install -y gcc-c++ autoconf automake libtool cmake svn svn2cl \
		openssl-devel boost-devel libpng-devel elfutils-devel
		;;
	SUSE*)
		echo 'SUSE detected. Installing required packages...'
		;;
	CentOS*6*)
		echo 'CentOS detected. Installing required packages...'
		yum install -y gcc-c++ svn subversion-svn2cl openssl-devel boost-devel \
		libpng-devel elfutils-devel
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
		fi
		
		if [[ $response_install == 'y' || $force_all == 1 ]]; then
			echo 'Getting cmake rpm (CentOS has old cmake version)'
			if [[ $(uname -m) == 'x86_64' ]]; then
				wget http://pkgs.repoforge.org/cmake/cmake-2.8.8-1.el6.rfx.x86_64.rpm && yum install -y cmake-2.8.8-1.el6.rfx.x86_64.rpm
				rm -f cmake-2.8.8-1.el6.rfx.x86_64.rpm
			else
				wget http://pkgs.repoforge.org/cmake/cmake-2.8.8-1.el6.rfx.i686.rpm && yum install -y cmake-2.8.8-1.el6.rfx.i686.rpm
				rm -f cmake-2.8.8-1.el6.rfx.i686.rpm
			fi
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
		fi
		
		if [[ $response_install == 'y' || $force_all == 1 ]]; then
			if [[ $(uname -m) == 'x86_64' ]]; then
				wget mirror.centos.org/centos/6/os/x86_64/Packages/autoconf-2.63-5.1.el6.noarch.rpm && yum install -y autoconf-2.63-5.1.el6.noarch.rpm
				rm -f autoconf-2.63-5.1.el6.noarch.rpm
			else
				wget mirror.centos.org/centos/6/os/i386/Packages/autoconf-2.63-5.1.el6.noarch.rpm && yum install -y autoconf-2.63-5.1.el6.noarch.rpm
				rm -f autoconf-2.63-5.1.el6.noarch.rpm
			fi
		fi
		
		
		yum list installed | grep -i automake
		if [ $? -eq 0 ]; then
			if [[ $force_all == 0 ]]; then
				echo -n 'automake is already installed. CentOS 6.4 uses an old version of automake. Do you wish to remove the current version and get a newer one? [y/N]'
				read response_remove
				echo ''
				if [[ $response_remove == 'y' ]]; then
					yum -y remove automake
					response_install='y'
				else
					echo -n 'Do you still wish to install the newer version? [y/N]'
					read response_install
				fi
			else
				yum -y remove automake
				response_install='y'
			fi
		fi
		
		if [[ $response_install == 'y' || $force_all == 1 ]]; then
			if [[ $(uname -m) == 'x86_64' ]]; then
				wget ftp://rpmfind.net/linux/centos/6/os/x86_64/Packages/automake-1.11.1-4.el6.noarch.rpm && yum install -y automake-1.11.1-4.el6.noarch.rpm
				rm -f automake-1.11.1-4.el6.noarch.rpm
			else
				wget ftp://rpmfind.net/linux/centos/6/os/i386/Packages/automake-1.11.1-4.el6.noarch.rpm && yum install -y automake-1.11.1-4.el6.noarch.rpm
				rm -f automake-1.11.1-4.el6.noarch.rpm
			fi
		fi	
		
		yum list installed | grep -i libtool
		if [ $? -eq 0 ]; then
			if [[ $force_all == 0 ]]; then
				echo -n 'libtool is already installed. CentOS 6.4 uses an old version of libtool. Do you wish to remove the current version and get a newer one? [y/N]'
				read response_remove
				echo ''
				if [[ $response_remove == 'y' ]]; then
					yum -y remove libtool
					response_install='y'
				else
					echo -n 'Do you still wish to install the newer version? [y/N]'
					read response_install
				fi
			else
				yum -y remove libtool
				response_install='y'
			fi
		fi
		
		if [[ $response_install == 'y' || $force_all == 1 ]]; then
			if [[ $(uname -m) == 'x86_64' ]]; then
				wget ftp://rpmfind.net/linux/centos/6/os/x86_64/Packages/libtool-2.2.6-15.5.el6.x86_64.rpm && yum install -y libtool-2.2.6-15.5.el6.x86_64.rpm
				rm -f libtool-2.2.6-15.5.el6.x86_64.rpm
			else
				wget ftp://rpmfind.net/linux/centos/6/os/i386/Packages/libtool-2.2.6-15.5.el6.i686.rpm && yum install -y libtool-2.2.6-15.5.el6.i686.rpm
				rm -f libtool-2.2.6-15.5.el6.i686.rpm
			fi
		fi
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