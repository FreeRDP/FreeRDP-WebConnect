#ifndef _VERSION_ISS_
#define _VERSION_ISS_

#ifndef __POPT_P__
# define private CStrings
# pragma parseroption -p+
#endif

#define ExeVersion(Str FileName) \
	Local[0] = 0, Local[1] = 0, Local[2] = 0, Local[3] = 0, \
	ParseVersion(FileName, Local[0], Local[1], Local[2], Local[3]), \
	DecodeVer(EncodeVer(Local[0], Local[1], Local[2], Local[3]), 2 + (((Local[2] + Local[3]) > 0) ? 1 : 0) + ((Local[3] > 0) ? 1 : 0))

#define ExeFullVersion(Str FileName) \
	Local[0] = GetFileVersion(FileName), Local[0]
	
#ifdef APPNAME
# ifdef APPVERSION
#  define APPVERNAME APPNAME + ' ' + APPVERSION
# else
#  ifdef APPEXE
#   define APPVERSION ExeVersion(APPEXE)
#   define APPFULLVER ExeFullVersion(APPEXE)
#   define APPVERNAME APPNAME + ' ' + APPVERSION
#   define APPFULLVERNAME APPNAME + ' ' + APPFULLVER
#  endif
# endif
# ifdef APPVERSION
#  define SETUPVNAME APPNAME + '-' + APPVERSION + '-Setup'
# endif
# ifdef APPFULLVER
#  define SETUPFVNAME APPNAME + '-' + APPFULLVER + '-Setup'
# endif
# define SETUPNAME APPNAME + '-Setup'
#endif

#ifdef CStrings
# pragma parseroption -p-
#endif

#endif
