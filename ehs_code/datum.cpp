/* $Id: datum.cpp 164 2013-04-11 08:44:21Z vaxpower $
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

#include "datum.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

Datum & Datum::operator = ( const Datum & other )
{
    sDatum = other.sDatum;
    return *this;
}

Datum & Datum::operator= ( unsigned long inUL )
{
    char psBuffer [ 100 ];
    sprintf ( psBuffer, "%lu", inUL );
    sDatum = psBuffer;
    return *this;
}

Datum & Datum::operator= ( long inLong )
{
    char psBuffer [ 100 ];
    sprintf ( psBuffer, "%ld", inLong );
    sDatum = psBuffer;
    return *this;
}

Datum & Datum::operator= ( unsigned int inUI )
{
    char psBuffer [ 100 ];
    sprintf ( psBuffer, "%u", inUI );
    sDatum = psBuffer;
    return *this;
}

Datum & Datum::operator= ( int inInt )
{
    char psBuffer [ 100 ];
    sprintf ( psBuffer, "%d", inInt );
    sDatum = psBuffer;
    return *this;
}

Datum & Datum::operator= ( double inDouble )
{
    char psBuffer [ 100 ];
    sprintf ( psBuffer, "%f", inDouble );
    sDatum = psBuffer;
    return *this;
}

Datum & Datum::operator= ( std::string isString )
{
    sDatum = isString;
    return *this;
}

Datum & Datum::operator= ( char * ipsString )
{
    sDatum = ipsString;
    return *this;
}

Datum & Datum::operator= ( const char * ipsString )
{
    sDatum = ipsString;
    return *this;
}

bool Datum::operator== ( const char * ipsString )
{
    return sDatum == ipsString;
}

Datum::operator unsigned long ( )
{
    return strtoul ( sDatum.c_str ( ) , NULL, 10);
}

Datum::operator long ( )
{
    return atol ( sDatum.c_str ( ) );
}

Datum::operator int ( )
{
    return atoi ( sDatum.c_str ( ) );
}

Datum::operator double ( )
{
    return atof ( sDatum.c_str ( ) );
}

Datum::operator std::string ( )
{
    return sDatum;
}

Datum::operator const char * ( )
{
    return sDatum.c_str ( );
}

bool Datum::operator!= ( int inCompare )
{
    return ((int)*this) != inCompare;
}

bool Datum::operator != ( const char * ipsCompare ) 
{
#ifdef _WIN32
	return _stricmp((const char*)*this, ipsCompare);
#else
	return strcasecmp((const char*)*this, ipsCompare);
#endif
}
