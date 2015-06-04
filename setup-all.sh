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
function change_branch()
{
	local REPO_PATH=$1
	local BRANCH=$2
	if [ ! -z "$BRANCH" ]; then
		local current=`pwd`
		cd $REPO_PATH
		git checkout $BRANCH
		cd $current
	fi
}
function git_clone_pull()
{
	local REPO_PATH=$1
	local REPO_URL=$2
	local BRANCH=$3
        if [ -d "$REPO_PATH" ]; then
		pushd .
		cd $REPO_PATH
		git pull
		change_branch $REPO_PATH $BRANCH
		popd
	else
		git clone $REPO_URL
		change_branch $REPO_PATH $BRANCH
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
# 8 = failed to build casablanca package
# 9 = failed to test casablanca build
# 10 = failed to install casablanca package
# 11 = failed to build FreeRDP-WebConnect package
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
			4) 	echo 'Unable to build ehs package. Exiting...'
				#cleanup
				;;
			5) 	echo "Unable to install ehs package into /usr. Exiting..."
				#cleanup
				;;
			6) 	echo 'Unable to build FreeRDP package. Exiting...'
				#cleanup
				;;
			7)	echo "Unable to install FreeRDP package into /usr. Exiting..."
				#cleanup
				;;
			8)	echo "Unable to build casablanca package. Exiting..."
				;;
			9)	echo "Testing the casablanca build failed. Exiting... "
				;;
			10)	echo "Unable to install casablanca package into /usr. Exiting..."
				;;
			11)	echo "Unable to build FreeRDP-WebConnect. Exiting..."
				#cleanup
				;;
			99) echo 'Internal error. Make sure you have an internet connection and that nothing is interfering with this script before running again (broken/rooted system or something deleting parts of the file-tree in mid-process).'
				#cleanup
				;;
			*)	echo 'Unknown error exit. Should not have happened.'
				;;
		esac
}

# trap exit errors and handle them in the exit_handler above
trap 'exit_handler ${LINENO} $?' EXIT

if [[ $# -gt 0 ]]; then
	# Get command-line options
	TEMP=`getopt -o fidch --long force-root,install-deps,--delete-packages,clean,help -q -- "$@"`
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
			-c|--clean) echo 'Temporarily removed clean-up'; exit 0;;
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

# Try sudo command. If sudo not present, try to su - to root.
command -v sudo >/dev/null 2>&1 && sudo_present=1

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
		if [[ $sudo_present -eq 1 ]]; then
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

# CERN devtoolset-2 on CentOS / RHEL / SL
if [ -d "/opt/rh/devtoolset-2/root/usr/bin" ]; then
	export PATH=/opt/rh/devtoolset-2/root/usr/bin:$PATH
fi

# Downloading ehs and FreeRDP source code in $HOME/prereqs
mkdir -p prereqs || exit 99
cd prereqs || exit 99
echo '---- Checking out ehs trunk code ----'
git_clone_pull EHS https://github.com/cloudbase/EHS.git || { echo 'Unable to download ehs from github'; exit 99; }
cd EHS || exit 99
echo '---- Starting ehs build ----'
mkdir -p build && cd build && cmake -DCMAKE_INSTALL_PREFIX=/usr .. && make || exit 4
echo '---- Finished building ehs ----'
if [[ $sudo_present -eq 1 ]]; then
	echo 'sudo available. Please enter your password to install ehs: '
	sudo make install || exit 5
else
	echo 'sudo command unavailable. Please enter root password to install ehs'
	su -c make install
fi
echo '---- Finished installing ehs ----'
cd ../.. || exit 99
echo '---- Checking out freerdp master ----'
git_clone_pull FreeRDP https://github.com/FreeRDP/FreeRDP.git stable-1.1 || { echo 'Unable to download FreeRDP'; exit 99; }
cd FreeRDP || exit 99
echo '---- Start installing freerdp ----'
mkdir -p build && cd build && cmake -DCMAKE_INSTALL_PREFIX=/usr .. || exit 6
echo '---- Building freerdp ----'
make || exit 6
echo '---- Finished building freerdp ----'
if [[ $sudo_present -eq 1 ]]; then
	echo 'sudo available. Please enter your password to install freerdp: '
	sudo make install || exit 7
	if [ -d /etc/ld.so.conf.d ]; then
		sudo touch /etc/ld.so.conf.d/freerdp.conf
		echo '/usr/lib/x86_64-linux-gnu' > ./freerdp.conf
		sudo mv ./freerdp.conf /etc/ld.so.conf.d/
		sudo ldconfig
	fi
else
	echo 'sudo command unavailable. Please enter root password to install freerdp'
	su -c make install || exit 7
	if [ -d /etc/ld.so.conf.d ]; then
		su -c touch /etc/ld.so.conf.d/freerdp.conf
		echo '/usr/lib/x86_64-linux-gnu' > ./freerdp.conf
		su -c mv ./freerdp.conf /etc/ld.so.conf.d/
		su -c ldconfig
	fi
fi
echo '---- Finished installing freerdp ----'
cd ../.. || exit 99
echo '---- Checking out casablanca master ----'
git_clone_pull casablanca https://git.codeplex.com/casablanca  || { echo 'Unable to download casablanca from codeplex'; exit 99; }
cd casablanca/Release || exit 99
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release . || exit 8
make || exit 8
#make test || exit 9
if [[ $sudo_present -eq 1 ]]; then
	echo 'sudo available. Please enter your password to install casablanca: '
	sudo cp Binaries/libcpprest.so /usr/lib || exit 10
	sudo ldconfig || exit 10
	sudo mkdir -p /usr/include/casablanca || exit 10
	sudo cp -r ../Release/include/* /usr/include/casablanca || exit 10
else
	echo 'sudo command unavailable. Please enter root password to install casablanca'
	su -c cp Binaries/libcpprest.so /usr/lib$BITNESS || exit 10
	su -c ldconfig || exit 10
	su -c mkdir -p /usr/include/casablanca || exit 10
	su -c cp -r ../Release/include/* /usr/include/casablanca || exit 10
fi
echo '---- Going back to webconnect ----'
popd
cd wsgate/ || exit 99
mkdir -p build || exit 99
cd build || exit 99
cmake .. || exit 11
echo '---- Building webconnect ----'
make || exit 11
echo "---- Built wsgate successfully in $PWD ----"
echo '---- Finished successfully ----'
