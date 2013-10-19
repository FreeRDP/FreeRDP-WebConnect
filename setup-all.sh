#!/bin/bash

# Script used to install FreeRDP-WebConnect on the local machine

# Exit status:
# 0 = Success
# 1 = improper command-line arguments
# 2 = executed by root, but no force-root option provided
# 3 = install dependencies failed
# 4 = failed to build ehs package
# 5 = failed to install ehs package
# 6 = failed to build FreeRDP package
# 7 = failed to install FreeRDP package
# 8 = failed to build FreeRDP-WebConnect package
# 99 = failed to execute some shell command

USAGE=$(basename $0) [-f|--force-root] [-i|--install-deps]
# Root is not supposed to run this script. If however -f or --force-root is given on the command-line
# this rule will not be enforced. Otherwise, the script will exit.

if [[ $# -gt 1 ]]; then
	# Get command-line options
	TEMP=`getopt -o fih --long force-root,install-deps,help -q -- "$@"`
	# getopt failed because of improper arguments. Terminate script.
	if [ $? != 0 ] ; then echo "$USAGE" >&2 ; exit 1 ; fi
	# preserve whitespace
	eval set -- "$TEMP"
	# Parse command-line arguments
	while true ; do
		case "$1" in
			-f|--force-root) force_root=1 ; shift ;;
			-i|--install-deps) instal_deps=1 ; shift ;;
			-h|--help) echo "$USAGE"; exit 0;;
			--) shift ; break ;;
			*) echo "Internal error while parsing command-line. Exiting..." ; exit 1 ;;
		esac
	done
fi

if [[ $UID -eq 0 && force_root -ne 1 ]]; then
   echo "If you wish to run this script as root, use the --force-root option."
   exit 2
fi

# Install_deps set
if [[ $install_deps -eq 1 ]]; then
	echo 'Preparing to install package dependencies'
	
	# If script executed by root and force_root enabled, then just install dependencies
	if [[ $UID -eq 0 && force_root -eq 1 ]]; then
		./install_prereqs.sh
		if [[ $? -ne 0 ]]; then
			echo 'Unable to install dependencies. '
			exit 3
		fi
	else
	# Try sudo command. If sudo not present, try to su - to root.
		command -v sudo >/dev/null 2>&1 && sudo_present=1
		if [[ sudo_present -eq 1 ]]; then
			echo 'sudo available. Please enter your password: '
			sudo ./install_prereqs.sh
			if [[ $? -ne 0 ]]; then
				exit 3
			fi
		else
			echo 'sudo command unavailable. Please enter root password: '
			su -c ./install_prereqs.sh
			if [[ $? -ne 0 ]]; then
				exit 3
			fi
		fi
	fi
fi

echo ---- Fetching webconnect dependencies into $(dirname `pwd`)/prereqs ----
cd .. || exit 99
mkdir -p prereqs || exit 99
mkdir -p $HOME/local || exit 99
cd prereqs || exit 99
echo '---- Checking out ehs trunk code ----'
svn checkout svn://svn.code.sf.net/p/ehs/code/trunk ehs-code || echo 'Failed to download ehs code from svn'; exit 99
cd ehs-code || exit 99
make -f Makefile.am || exit 4
./configure --with-ssl --prefix=$HOME/local || exit 4
echo '---- Starting ehs build ----'
make || exit 4
echo '---- Finished building ehs ----'
make install || exit 5
echo '---- Finished installing ehs ----'
cd .. || exit 99
echo '---- Checking out freerdp master ----'
git clone https://github.com/FreeRDP/FreeRDP.git || echo 'Failed to download FreeRDP code from github.'; exit 99
cd FreeRDP || exit 99
mkdir -p build && cd build && cmake -DCMAKE_INSTALL_PREFIX=$HOME/local .. -DLIB_SUFFIX=64 || exit 6
echo '---- Building freerdp ----'
make || exit 6
echo '---- Finished building freerdp ----'
make install || exit 7
echo '---- Finished installing freerdp ----'
echo '---- Going back to webconnect ----'
cd ../../../FreeRDP-WebConnect/wsgate/ || exit 99
make -f Makefile.am || exit 8
./configure || exit 8
echo '---- Building webconnect ----'
make || exit 8
echo '---- Finished successfully ----'
echo '---- To run please use `cd wsgate && ./wsgate -c wsgate.mrd.ini` and connect on localhost:8888 ----'