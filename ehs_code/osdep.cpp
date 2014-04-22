/* $Id: osdep.cpp 84 2012-03-21 10:52:20Z felfert $
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef _WIN32
# include "ehstypes.h"

ehs_threadid_t THREADID(pthread_t t) {
# define _NOT_IMPLEMENTED
# ifdef HAVE_PTHREAD_GETW32THREADID_NP
    return pthread_getw32threadid_np(t);
#  undef _NOT_IMPLEMENTED
# endif
# ifdef HAVE_PTHREAD_GETW32THREADHANDLE_NP
    return (unsigned long)pthread_getw32threadhandle_np(t);
#  undef _NOT_IMPLEMENTED
# endif
# ifdef _NOT_IMPLEMENTED
#  error No suitable function for retrieving a thread-ID on this platform
# endif
}
#endif
