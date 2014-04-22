/* $Id: debug.h 81 2012-03-20 12:03:05Z felfert $
 *
 * EHS is a library for embedding HTTP(S) support into a C++ application
 *
 * Copyright (C) 2004 Zachary J. Hansen
 *
 * Code cleanup, new features and bugfixes: Copyright (C) 2010 Fritz Elfert
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License version 2.1 as published by the Free Software Foundation;
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with this library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    This can be found in the 'COPYING' file.
 *
 */

#ifndef _DEBUG_H
#define _DEBUG_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef EHS_DEBUG
# include <iostream>
# include <cstdio>
# include <cstdarg>
#ifdef WIN32
#include <debugapi.h>
#endif
#endif

static inline void _ehs_trace(const char*
#ifdef EHS_DEBUG
        szFormat
#endif
        ... )
{
#ifdef EHS_DEBUG
    const int bufsize = 100000; 
    char buf [ bufsize ] ; 
    va_list VarList;
    va_start( VarList, szFormat );
    vsnprintf(buf, bufsize - 1, szFormat, VarList ) ;
    va_end ( VarList ) ;
# ifdef _WIN32
    OutputDebugStringA(buf) ;
# else
    std::cerr << buf << std::endl; std::cerr.flush();
# endif
#endif
}

#define _STR(x) #x
#define EHS_TODO       (_message) message ("  *** TODO: " ##_message "\t\t\t\t" __FILE__ ":" _STR(__LINE__) )	
#define EHS_FUTURE     (_message) message ("  *** FUTURE: " ##_message "\t\t\t\t" __FILE__ ":" _STR(__LINE__) )	
#define EHS_TODOCUMENT (_message) message ("  *** TODOCUMENT: " ##_message "\t\t\t\t" __FILE__ ":" _STR(__LINE__) )	
#define EHS_DEBUGCODE  (_message) message ("  *** DEBUG CODE (REMOVE!): " ##_message "\t\t\t\t" __FILE__ ":" _STR(__LINE__) )	

#ifdef HAVE_GNU_VAMACROS
# ifdef HAVE_GNU_PRETTY_FUNCTION
#  define EHS_TRACE(fmt, ...) _ehs_trace("%s [%s:%d]: " fmt, __PRETTY_FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)
# else
#  define EHS_TRACE(fmt, ...) _ehs_trace("%s [%s:%d]: " fmt, __func__, __FILE__, __LINE__, ##__VA_ARGS__)
# endif
#else
# define EHS_TRACE _ehs_trace
#endif

#endif // _DEBUG_H
