dnl Process this file with autoconf to produce a configure script.
AC_INIT([wsgate],[1.0.0],[fritz@fritz-elfert.de])
AC_REVISION([$Id$])
AC_CONFIG_AUX_DIR([conf])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([wsgate_main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign no-installinfo no-installman])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

case $build_os in
    mingw*)
        if test "$build_os" = "$target_os" ; then
            # On native MinGW, the default include/lib paths
            # are /mingw/include resp. /mingw/lib, however
            # several contributed stuff is under /usr/...
            CPPFLAGS="$CPPFLAGS -I/usr/include"
            LDFLAGS="$LDFLAGS -L/usr/lib"
            # On native MinGW, we also want a prefix of /,
            # because we want to install directly in our
            # setupdir's bin (see packaging/win32/Makefile.am)
            if test "x$prefix" = xNONE; then
                prefix=/
            fi
        fi
        ;;
esac

SERVICE_OBJECTS=
WSGATE_RES=
PLATFORM_SUBDIRS=
case $target_os in
    mingw*)
        PLATFORM_SUBDIRS=setup
        WSGATE_RES='wsgate.$(OBJEXT)'
        SERVICE_OBJECTS='NTService.$(OBJEXT)'
        ;;
esac
AC_SUBST(PLATFORM_SUBDIRS)
AC_SUBST(WSGATE_RES)
AC_SUBST(SERVICE_OBJECTS)

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

m4_ifdef([LT_INIT],[
LT_INIT([win32-dll pic-only])
case $target_os in
    mingw*)
        LT_LANG([RC])
        AC_CHECK_TOOL(RC,windres,)
        AC_CHECK_TOOL(MC,windmc,)
        ;;
esac
],[
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
case $target_os in
    mingw*)
        tagnames="RC"
        _LT_AC_TAGCONFIG
        AC_CHECK_TOOL(RC,windres,)
        AC_CHECK_TOOL(MC,windmc,)
        ;;
esac
])

dnl Checks for libraries.
ac_cv_dllpath=
AC_ARG_WITH([dllpath],
            AS_HELP_STRING([--with-dllpath=PATH],
                           [Specify path for DLLs (mingw32 crossbuild only)]),
        [ac_cv_dllpath=$withval])


BFDLIB1=
BFDLIB2=
DWLIB=

dnl The following headers have to be checked *before* invoking
dnl AC_CHECK_LIB for ssl and crypto in order for those
dnl checks to work correctly in mingw
AC_CHECK_HEADERS([openssl/ssl.h openssl/err.h])

WSGATE_LDADD=
AC_CHECK_LIB(pthread, pthread_create,[WSGATE_LDADD="$WSGATE_LDADD -lpthread"])
AC_CHECK_LIB(sokt, socket,[WSGATE_LDADD="$WSGATE_LDADD -lsokt"])
AC_CHECK_LIB(z, inflate,[WSGATE_LDADD="$WSGATE_LDADD -lz"])
AC_CHECK_LIB(crypto, ERR_free_strings,[WSGATE_LDADD="$WSGATE_LDADD -lcrypto"])
AC_CHECK_LIB(ssl, SSL_library_init,[WSGATE_LDADD="$WSGATE_LDADD -lssl"])
AC_CHECK_LIB(ws2_32, main,[WSGATE_LDADD="$WSGATE_LDADD -lws2_32"])
AC_CHECK_LIB(dw, dwfl_begin, DWLIB="-ldw")
AC_CHECK_LIB(bfd, bfd_openr, [BFDLIB1="-lbfd"])
AC_CHECK_LIB(iberty, cplus_demangle, [BFDLIB2="-liberty"])
havessl=$ac_cv_lib_ssl_SSL_library_init$ac_cv_lib_crypto_ERR_free_strings

AC_MSG_CHECKING([whether to build with SSL support])
withval=AUTO
AC_ARG_WITH([ssl],AS_HELP_STRING([--with-ssl],[compile with SSL support (requires OpenSSL)]))
AC_MSG_RESULT([$withval])
case "$withval" in
    [[yY][eE][sS]])
        test "$havessl" = "yesyes" || AC_MSG_ERROR([For compiling with SSL support, OpenSSL is required])
        AC_DEFINE([COMPILE_WITH_SSL],[1],[Whether to compile with SSL support or not])
        ;;
    AUTO)
        if test "$havessl" = "yesyes" ; then
            AC_DEFINE([COMPILE_WITH_SSL],[1],[Whether to compile with SSL support or not])
            AC_MSG_RESULT([configure: OpenSSL is available, enabled SSL support])
        else
            AC_MSG_RESULT([configure: OpenSSL not available, disabled SSL support])
        fi
        ;;
       *)
        if test -d "${withval}/include/openssl" -a "${withval}/lib" ; then
            AC_DEFINE([COMPILE_WITH_SSL],[1],[Whether to compile with SSL support or not])
            CPPFLAGS="-I${withval}/include $CPPFLAGS"
            LDFLAGS="-L${withval}/lib"
        fi
        ;;
esac

AC_MSG_CHECKING([whether to build with debugging information])
enableval=NO
AC_ARG_ENABLE([debug],AS_HELP_STRING([--enable-debug],[compile with debugging information]))
AC_MSG_RESULT([$enableval])
case "$enableval" in
    [[yY][eE][sS]])
        AC_DEFINE([WSGATE_DEBUG],[1],[Whether to compile with debugging information])
        CFLAGS="$CFLAGS -O0"
        CXXFLAGS="$CXXFLAGS -O0"
        ;;
esac

AC_MSG_CHECKING([whether to prefer BFD over DWARF])
enableval=NO
PREFER_BFD=NO
AC_ARG_ENABLE(bfd,AS_HELP_STRING([--enable-bfd],[prefer BFD over DWARF)]))
AC_MSG_RESULT([$enableval])
case "$enableval" in
    [[yY][eE][sS]])
        PREFER_BFD=YES
        ;;
esac

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h demangle.h dwarf.h fcntl.h netinet/in.h signal.h stdlib.h string.h syslog.h sys/ioctl.h sys/resource.h sys/socket.h sys/stat.h sys/time.h sys/types.h sys/wait.h termios.h time.h unistd.h execinfo.h conio.h winsock2.h windows.h])

dnl DWARF vs. BFD
DW_CPPFLAGS=
AC_CHECK_HEADERS([libdwfl.h])
if test "$ac_cv_header_libdwfl_h" = "no" ; then
    AC_CHECK_HEADERS([elfutils/libdwfl.h],[DW_CPPFLAGS="-I/usr/include/elfutils"])
fi

if test -n "$DWLIB" -a "$PREFER_BFD" = "NO" ; then
    TRACE_LIBS="$DWLIB"
    TRACE_CPPFLAGS="$DW_CPPFLAGS"
    AC_DEFINE_UNQUOTED([USE_DWARF],1,[Define, if using libdw for resolving symbols])
    AC_MSG_NOTICE([Using DWARF for resolving symbols])
    if test -n "$BFDLIB2" ; then
        TRACE_LIBS="$DWLIB $BFDLIB2"
        AC_DEFINE_UNQUOTED([HAVE_LIBIBERTY],1,[Define, if libiberty is available])
    fi
else
    if test -n "$BFDLIB1" -a -n "$BFDLIB2" ; then
        TRACE_LIBS="$BFDLIB1 $BFDLIB2"
        AC_DEFINE_UNQUOTED([USE_BFD],1,[Define, if using bfd for resolving symbols])
        AC_MSG_NOTICE([Using BFD for resolving symbols])
    else
        AC_MSG_NOTICE([Disabling backtracing])
    fi
fi
AC_SUBST(TRACE_LIBS)
AC_SUBST(TRACE_CPPFLAGS)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_PID_T
AC_C_CONST

INN_C_GNU_VAMACROS
INN_C_GNU_PRETTY_FUNCTION

dnl Checks for library functions.
AC_FUNC_VPRINTF
AC_FUNC_FORK
AC_CHECK_FUNCS([inet_ntoa memset select setlocale socket strcasecmp strerror strtoul pthread_getw32threadid_np pthread_getw32threadhandle_np])
tmp_LIBS="$LIBS"
LIBS="$LIBS $BFDLIB1 $BFDLIB2"
AC_CHECK_FUNCS([bfd_demangle])
LIBS="$tmp_LIBS"

FE_GITREV([GITREV])
AC_DEFINE_UNQUOTED(GITREV,"$GITREV",[GIT revision])
FE_RCVERSION

dnl Use modern -pthread gcc option if available.
fe_CHECK_FLAG([C++],[CXXFLAGS],[-pthread])

dnl Use -fPIE and -pie for suid bindhelper
fe_CHECK_FLAG([C],[SUID_CFLAGS],[-fPIC])
fe_CHECK_FLAG([C],[SUID_LDFLAGS],[-fpic])
SUID_CFLAGS=$ac_checked_SUID_CFLGAGS
SUID_LDFLAGS=$ac_checked_SUID_LDFLAGS
AC_SUBST(SUID_CFLAGS)
AC_SUBST(SUID_LDFLAGS)

AC_ARG_ENABLE([warn],
	AS_HELP_STRING([--enable-warn],
		[Enable additional warnings (Default: NO)]),
	[
		AC_MSG_NOTICE([will enable additional warnings])
        fe_CHECK_FLAG([C],[CFLAGS],[-Wimplicit])
        CFLAGS="$CFLAGS $ac_checked_CFLAGS"
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-pedantic])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wall])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wextra])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wchar-subscripts])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wcomment])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wformat])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wformat-security])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wmissing-braces])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wparentheses])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wreturn-type])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wswitch])
        fe_CHECK_FLAG([C++],[CPPFLAGS],[-Wuninitialized])
        CPPFLAGS="$CPPFLAGS $ac_checked_CPPFLAGS"
        fe_CHECK_FLAG([C++],[CXXFLAGS],[-Wctor-dtor-privacy])
        fe_CHECK_FLAG([C++],[CXXFLAGS],[-Wnon-virtual-dtor])
        fe_CHECK_FLAG([C++],[CXXFLAGS],[-Woverloaded-virtual])
        fe_CHECK_FLAG([C++],[CXXFLAGS],[-Weffc++])
        CXXFLAGS="$CXXFLAGS $ac_checked_CXXFLAGS"
	]
)
LIBVERSION="`echo $VERSION|tr . :`"
AC_SUBST(LIBVERSION)

dnl Disable strict aliasing
ac_checked_CXXFLAGS=
fe_CHECK_FLAG([C++],[CXXFLAGS],[-fno-strict-aliasing])
CXXFLAGS="$CXXFLAGS $ac_checked_CXXFLAGS"

dnl Check for boost minimum version
BOOST_STATIC
BOOST_REQUIRE([1.34.0])
BOOST_REGEX([mt])
BOOST_PROGRAM_OPTIONS([mt])
BOOST_FILESYSTEM([mt])

dnl Check for boost::lock_guard

AC_MSG_CHECKING([for availability of boost::lock_guard])
AC_LANG_PUSH([C++])
AC_LINK_IFELSE([
AC_LANG_PROGRAM([[#include <boost/thread.hpp>
boost::recursive_mutex m;]],
[[boost::lock_guard<boost::recursive_mutex> lock(m);]]
)],[AC_DEFINE([HAVE_BOOST_LOCK_GUARD],[1],[Define if boost::lock_guard is available])
    AC_MSG_RESULT([yes])],[AC_MSG_RESULT([no])])
AC_LANG_POP

dnl Check for std::unique_ptr and std::move
ac_checked_CXXFLAGS=
fe_CHECK_FLAG([C++],[CXXFLAGS],[-std=gnu++0x])
CXXFLAGS="$CXXFLAGS $ac_checked_CXXFLAGS"
AC_MSG_CHECKING([for availability of std::unique_ptr and std::move])
USE_UNIQUE_PTR=0
AC_LANG_PUSH([C++])
AC_COMPILE_IFELSE([
AC_LANG_PROGRAM([[#include <memory>
class foo { public: int bar; };
void mv(std::unique_ptr<foo>p) { int v = p->bar; }]],
[[std::unique_ptr<foo>p(new foo());mv(std::move(p));]]
)],[USE_UNIQUE_PTR=1;AC_MSG_RESULT([yes])],[AC_MSG_RESULT([no])])
AC_LANG_POP
AC_SUBST([USE_UNIQUE_PTR])

CPPFLAGS="-I/usr/include/ehs $CPPFLAGS"
AC_LANG_PUSH([C++])
AC_CHECK_LIB(ehs,getEHSconfig,[WSGATE_LDADD="$WSGATE_LDADD -lehs"],,$BOOST_REGEX_LIBS)
AC_CHECK_HEADERS([ehs.h])
AC_LANG_POP

if test "$ac_cv_lib_ehs_getEHSconfig$ac_cv_header_ehs_h" != "yesyes" ; then
    AC_MSG_ERROR([Mandatory EHS development package not found])
fi
if test "$ac_cv_header_elfutils_libdwfl_h" = "yes" ; then
CPPFLAGS="-I/usr/include/elfutils $CPPFLAGS"
fi

dnl FreeRDP
AC_CHECK_LIB([freerdp-core],[freerdp_input_send_keyboard_event],
    [WSGATE_LDADD="$WSGATE_LDADD -lfreerdp-core"],
    [AC_MSG_ERROR([You need freerdp from GIT!])])
AC_CHECK_LIB([freerdp-codec],[freerdp_color_convert_var],
    [WSGATE_LDADD="$WSGATE_LDADD -lfreerdp-codec"],
    [AC_MSG_ERROR([You need freerdp from GIT!])])
AC_CHECK_LIB([freerdp-gdi],[gdi_init],
    [WSGATE_LDADD="$WSGATE_LDADD -lfreerdp-gdi"],
    [AC_MSG_ERROR([You need freerdp from GIT!])])
AC_CHECK_LIB([freerdp-locale],[freerdp_keyboard_get_rdp_scancode_from_virtual_key_code],
    [WSGATE_LDADD="$WSGATE_LDADD -lfreerdp-locale"],
    [AC_MSG_ERROR([You need freerdp from GIT!])])
AC_CHECK_LIB([freerdp-cache],[pointer_cache_register_callbacks],
    [WSGATE_LDADD="$WSGATE_LDADD -lfreerdp-cache"],
    [AC_MSG_ERROR([You need freerdp from GIT!])])

dnl Test for libpng
png_ok=no
AC_LANG_PUSH([C])
AC_CHECK_LIB([png],[png_read_info],AC_CHECK_HEADER([png.h],[png_ok=yes]),,[-lz -lm])
if test "$png_ok" = yes; then
    AC_MSG_CHECKING([for png_structp in png.h])
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <png.h>]],
            [[png_structp pp; png_infop info; png_colorp cmap; png_create_read_struct;]])]
    ,,[png_ok=no])
    AC_MSG_RESULT($png_ok)
fi
AC_LANG_POP
if test "$png_ok" = yes; then
    LIBPNG='-lpng -lz'
else
    AC_MSG_ERROR([Mandatory libpng is not available])
fi
AC_SUBST(LIBPNG)

dnl Required for js and css compression: java
AC_CHECK_TOOL([JAVA],[java],[])
JS_COMPRESS='$(JAVA) -jar $(top_srcdir)/tools/yuicompressor.jar --type js'
CSS_COMPRESS='$(JAVA) -jar $(top_srcdir)/tools/yuicompressor.jar --type css'
if test -z "${JAVA}" ; then
    JS_COMPRESS=cat
    CSS_COMPRESS=cat
fi
AC_SUBST(JS_COMPRESS)
AC_SUBST(CSS_COMPRESS)

dnl Required for findrequires.pl script
AC_CHECK_TARGET_TOOL([OBJDUMP],[objdump],[no])
case ${OBJDUMP}-${target_os} in
    no-mingw*)
        AC_MSG_ERROR([Could not find required tool 'objdump'])
        ;;
esac
AC_ARG_VAR([OBJDUMP],[Path to objdump binary. (required for mingw32 builds only)])
dnl For the same reason, we require perl
STRIP_DEBUG='$(PERL) $(top_srcdir)/tools/strip-debug.pl'
AC_CHECK_TOOL([PERL],[perl],[no])
case ${PERL}-${target_os} in
    no-mingw*)
        AC_MSG_ERROR([Could not find required tool 'perl'])
        ;;
    no-*)
        STRIP_DEBUG='cat'
        ;;
esac
AC_SUBST(STRIP_DEBUG)

# ISCC env variable
AC_ARG_VAR([ISCC],[Full path of InnoSetup compiler iscc.exe.])dnl
SETUP=
DLLPATH=
PLATFORMDIST=
INSTALL_SAMPLES=
BINDHELPER='bindhelper$(EXEEXT)'
KEYGEN='conf/keygen.sh'

case $target_os in
    linux*)
        AC_CHECK_TOOL([RPMBUILD], [rpmbuild], [no])
        if test "$RPMBUILD" != "no" ; then
            PLATFORMDIST=rpm
        else
            AC_CHECK_TOOL([DPKG_BUILDPACKAGE], [dpkg-buildpackage], [no])
            if test "$DPKG_BUILDPACKAGE" != "no" ; then
                PLATFORMDIST=deb
            fi
        fi
        ;;
    mingw*)
        AC_DEFINE_UNQUOTED([_WIN32_WINNT],0x0501,[Minimum Windows version (XP)])
        AC_DEFINE_UNQUOTED([_UNICODE],1,[Select wide string variants])
        BINDHELPER=
        KEYGEN=
        PLATFORMDIST=setup
        INSTALL_SAMPLES='$(noinst_PROGRAMS)'
        if test "$build_os" = "$target_os" ; then
            SETUP=nativesetup
            DLLPATH="$PATH"
            AC_ARG_WITH([iscc],
                        AS_HELP_STRING([--with-iscc=PATH],[Override ISCC path. (native mingw only)]),
                        [ISCC="$withval"])
            AC_PATH_PROG([ISCC],[iscc],[""],[$PATH])
            if test -z "$ISCC" ; then
                AC_MSG_WARN([InnoSetup compiler is missing, therefore no Setup will be built!])
                SETUP=
            fi
        else
            SETUP=crosssetup
            if test -z "$ac_cv_dllpath" ; then
                AC_MSG_WARN([No --with-dllpath option specified, therefore no Setup will be built!])
                SETUP=
            else
                DLLPATH="$ac_cv_dllpath"
            fi
            AC_ARG_WITH([wine-iscc],
                        AS_HELP_STRING([--with-wine-iscc=PATH],[Path of ISCC wrapper to be run under wine. (mingw32 crossbuild only)]),
                        [ISCC="$withval"],[AC_CHECK_TOOL([ISCC],[iscc],[])])
            if test -n "$SETUP" ; then
                if test -z "$ISCC" ; then
                    AC_MSG_WARN([InnoSetup compiler is missing, therefore no Setup will be built!])
                    SETUP=
                else
                    AC_PATH_PROG([WINE],[wine],[""],[$PATH])
                    if test -z "$WINE" ; then
                        AC_MSG_WARN([wine is not available, therefore no Setup will be built!])
                        SETUP=
                    fi
                fi
            fi
        fi
        ;;
esac

AC_SUBST(KEYGEN)
AC_SUBST(BINDHELPER)
AC_SUBST(SETUP)
AC_SUBST(DLLPATH)
AC_SUBST(PLATFORMDIST)
AC_SUBST(INSTALL_SAMPLES)
WSGATE_LDADD="$WSGATE_LDADD  $BOOST_REGEX_LIBS $BOOST_FILESYSTEM_LIBS $BOOST_SYSTEM_LIBS $BOOST_PROGRAM_OPTIONS_LIBS $TRACE_LIBS"
AC_SUBST(WSGATE_LDADD)

dnl
dnl doxygen setup
dnl
DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN([wsgate], [conf/doxygen.cfg], [doc])

AC_CONFIG_FILES([Makefile setup/Makefile webroot/Makefile
                 conf/doxygen.cfg conf/wsgate.spec
                 setup/findrequires.pl setup/isccwrap.sh wsgate.rc])
AC_OUTPUT
