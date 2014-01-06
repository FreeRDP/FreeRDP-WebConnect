#!/bin/bash

# Script used to install FreeRDP-WebConnect on the local machine

# Usage indications
USAGE="$(basename $0) [-f|--force-root] [[-i|--install-deps] -d|--delete-packages] [-c|--clean] [-h|--help]"

# determine if we are running on 32 or 64 bit
if [[ $(uname -m) == 'x86_64' ]]; then
	BITNESS=64
else
	BITNESS=''
fi

# clean-up ehs and freerdp files and folders. Is automatically invoked if the script encounters an error.

function cleanup()
{
	if [ -d $HOME/prereqs ]; then
		rm -rf $HOME/prereqs
	fi
	if [ -d $HOME/local/lib$BITNESS ]; then
		rm -f $HOME/local/lib$BITNESS/lib*freerdp*.so*
		rm -f $HOME/local/lib$BITNESS/libwinpr*.so*
		rm -f $HOME/local/lib$BITNESS/libwinpr*.a
		rm -f $HOME/local/lib$BITNESS/libehs*.so*
		rm -f $HOME/local/lib$BITNESS/libehs.*a
		rm -f $HOME/local/lib$BITNESS/pkgconfig/freerdp.pc
	fi
	if [ -d $HOME/local/include/ehs ]; then
		rm -rf $HOME/local/include/ehs
	fi
	if [ -d $HOME/local/include/freerdp ]; then
		rm -rf $HOME/local/include/freerdp
	fi
	if [ -d $HOME/local/include/winpr ]; then
		rm -rf $HOME/local/include/winpr
	fi
}

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

# trap handler: print location of last error and process it further
#
function exit_handler()
{
        MYSELF="$0"               # equals to my script name
        LASTLINE="$1"            # argument 1: last line of error occurence
        LASTERR="$2"             # argument 2: error code of last command
        #echo "${MYSELF}: line ${LASTLINE}: exit status of last command: ${LASTERR}"

        case ${LASTERR} in
			0)	;;
			1)	echo ${USAGE}
				;;
			2) 	echo "If you wish to run this script as root, use the --force-root option."
				echo ${USAGE}
				;;
			3) 	echo 'Unable to install dependencies. Try to manually install packages listed in install_prereqs.sh according to your distribution.'
				echo 'After that, run the script without the --install-deps flag'
				;;
			4) 	echo 'Unable to build ehs package. Cleaning up and exiting...'
				cleanup
				;;
			5) 	echo "Unable to install ehs package into $HOME/local. Cleaning up and exiting..."
				cleanup
				;;
			6) 	echo 'Unable to build FreeRDP package. Cleaning up and exiting...'
				cleanup
				;;
			7)	echo "Unable to install FreeRDP package into $HOME/local. Cleaning up and exiting..."
				cleanup
				;;
			8)	echo "Unable to build FreeRDP-WebConnect. Cleaning up and exiting..."
				cleanup
				;;
			99) echo 'Internal error. Make sure you have an internet connection and that nothing is interfering with this script before running again (broken/rooted system or something deleting parts of the file-tree in mid-process).'
				cleanup
				;;
			*)	echo 'Unknown error exit. Should not have happened.'
				;;
		esac
}

# trap exit errors and handle them in the exit_handler above
trap 'exit_handler ${LINENO} $?' EXIT

if [[ $# -gt 0 ]]; then
	# Get command-line options
	TEMP=`getopt -o fich --long force-root,install-deps,clean,help -q -- "$@"`
	# getopt failed because of improper arguments. Terminate script.
	if [ $? != 0 ] ; then echo "$USAGE" >&2 ; exit 1 ; fi
	# preserve whitespace
	eval set -- "$TEMP"
	# Parse command-line arguments
	while true ; do
		case "$1" in
			-f|--force-root) force_root=1 ; shift ;;
			-i|--install-deps) install_deps=1 ; shift ;;
			-d|--delete-packages) delete_packages=1 ; shift ;;
			-h|--help) echo "$USAGE"; exit 0;;
			-c|--clean) echo 'Cleaning up'; cleanup; exit 0;;
			--) shift ; break ;;
			*) echo "Internal error while parsing command-line. Exiting..." ; exit 1 ;;
		esac
	done
fi

# Root is not supposed to run this script. If however -f or --force-root is given on the command-line
# this rule will not be enforced. Otherwise, the script will exit.
if [[ $UID -eq 0 && $force_root -ne 1 ]]; then
   exit 2
fi

if [[ $UID -ne 0 && $force_root -eq 1 ]]; then
   echo 'Warning: You do not need to specify the -f|--force-root flag if you do not run the script as root.'
fi

if [[ $delete_packages -eq 1 && $install_deps -ne 1 ]]; then
	echo 'Warning: You do not need to specify the -d|--delete-packages flag if you do not specify the -i|--install-deps flag.'
fi

# Install_deps set
if [[ $install_deps -eq 1 ]]; then
	echo 'Preparing to install package dependencies'
	
	# If script executed by root and force_root enabled, then just install dependencies
	if [[ $UID -eq 0 && $force_root -eq 1 ]]; then
		if [[ $delete_packages -eq 1 ]]; then
			./install_prereqs.sh -y
		else
			./install_prereqs.sh
		fi
		
		if [[ $? -ne 0 ]]; then
			exit 3
		fi
	else
	# Try sudo command. If sudo not present, try to su - to root.
		command -v sudo >/dev/null 2>&1 && sudo_present=1
		if [[ sudo_present -eq 1 ]]; then
			echo 'sudo available. Please enter your password: '
			if [[ $delete_packages -eq 1 ]]; then
				sudo ./install_prereqs.sh -y
			else
				sudo ./install_prereqs.sh
			fi
			
			if [[ $? -ne 0 ]]; then
				echo 'sudo failed. Trying to do it with root. Please enter root password: '
				if [[ $delete_packages -eq 1 ]]; then
					su -c ./install_prereqs.sh -y
				else
					su -c ./install_prereqs.sh
				fi
				
				if [[ $? -ne 0 ]]; then
					exit 3
				fi
			fi
		else
			echo 'sudo command unavailable. Please enter root password: '
			if [[ $delete_packages -eq 1 ]]; then
				su -c ./install_prereqs.sh -y
			else
				su -c ./install_prereqs.sh
			fi
			
			if [[ $? -ne 0 ]]; then
				exit 3
			fi
		fi
	fi
fi

echo "---- Fetching webconnect dependencies ehs and FreeRDP into $HOME/prereqs ----"
pushd $HOME || exit 99

# Downloading ehs and FreeRDP source code in $HOME/prereqs
mkdir -p prereqs || exit 99
# Installing ehs and FreeRDP libs and headers into $HOME/local
mkdir -p local || exit 99
cd prereqs || exit 99
echo '---- Checking out ehs trunk code ----'
svn checkout svn://svn.code.sf.net/p/ehs/code/trunk ehs-code || { echo 'Unable to download ehs from svn'; exit 99; }
cd ehs-code || exit 99
make -f Makefile.am || exit 4
./configure --with-ssl --prefix=$HOME/local --libdir=$HOME/local/lib$BITNESS || exit 4
echo '---- Starting ehs build ----'
make || exit 4
echo '---- Finished building ehs ----'
make install || exit 5
echo '---- Finished installing ehs ----'
cd .. || exit 99
echo '---- Checking out freerdp master ----'
git clone https://github.com/FreeRDP/FreeRDP.git || { echo 'Unable to download FreeRDP from github'; exit 99; }
cd FreeRDP || exit 99
mkdir -p build && cd build && cmake -DCMAKE_INSTALL_PREFIX=$HOME/local .. || exit 6
echo '---- Building freerdp ----'
make || exit 6
echo '---- Finished building freerdp ----'
make install || exit 7
echo '---- Finished installing freerdp ----'
echo '---- Going back to webconnect ----'
popd
cd wsgate/ || exit 99
make -f Makefile.am || exit 8
./configure || exit 8
echo '---- Building webconnect ----'
make || exit 8
echo '---- Finished successfully ----'
echo '---- To run please use `cd wsgate && ./wsgate -c wsgate.mrd.ini` and connect on localhost:8888 ----'