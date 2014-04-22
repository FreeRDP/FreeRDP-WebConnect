/* $Id: datum.h 164 2013-04-11 08:44:21Z vaxpower $
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

#ifndef DATUM_H
#define DATUM_H

#include <string>

/// class that makes it easy to go between numbers and strings
/** 
 * class that can take data in string or number form and can return it 
 * in either form.  Very similar to a perl scalar (without references)
 */
class Datum {

    protected:

        /// holds the data in string form
        std::string sDatum;

    public:
        /// Default constructor
        Datum ( ) : sDatum ( "" ) { }

        /// Copy constructor
        Datum ( const Datum & other ) : sDatum( other.sDatum ) { }

        /// Assignment operator
        Datum & operator=( const Datum & other );

        /// assignment operator for unsigned long
        Datum & operator= ( unsigned long inUL );

        /// assignment operator for longs
        Datum & operator= ( long inLong );

        /// assignment operator for unsigned int
        Datum & operator= ( unsigned int inUI );

        /// assignment operator for ints
        Datum & operator= ( int inInt );

        /// assignment operator for doubles
        Datum & operator= ( double inDouble );

        /// assignment operator for std::strings
        Datum & operator= ( std::string isString );

        /// assignment operator for char * strings
        Datum & operator= ( char * ipsString );

        /// assignment operator for const char * strings
        Datum & operator= ( const char * ipsString );

        /// assignment operator for void *
        Datum & operator= ( void * ipVoid );

        /// equality operator for const char * string
        bool operator== ( const char * ipsString );

        /// conversion operaor to return an in
        operator int ( );

        /// explicit accessor for int
        int GetInt() { return (int)(*this); }

        /// conversion operator for unsigned long
        operator unsigned long ( );

        /// conversion operator for long
        operator long ( );

        /// conversion operator for double
        operator double ( );

        /// conversion operator for std::string
        operator std::string ( );

        /// conversion operator for const char *
        operator const char * ( );

        /// explicit accessor for const char *
        const char * GetCharString ( ) { return (const char*)(*this); }


        /// inequality operator to test against int
        bool operator!= ( int );

        /// inequality operator to test against const char *
        bool operator!= ( const char * );

};

#endif // DATUM_H
