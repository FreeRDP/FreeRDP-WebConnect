/* $Id: contentdisposition.h 30 2010-06-05 19:08:00Z felfert $
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

#ifndef CONTENTDISPOSITION_H
#define CONTENTDISPOSITION_H

#include <string>
#include "ehstypes.h"

/**
 * This class stores the content disposition of a subbody.
 * It is used for multi-part form data.
 */
class ContentDisposition {

    public:

        /**
         * Default constructor.
         */
        ContentDisposition() :
            m_oContentDispositionHeaders(StringCaseMap()),
            m_sContentDisposition ("")
        {
        }

        /// Destructor
        ~ContentDisposition ( ) {
        }

        /// Map of headers for this content disposition.
        StringCaseMap m_oContentDispositionHeaders;

        /// Content disposition string for this object
        std::string m_sContentDisposition;
};

#endif // CONTENTDISPOSITION_H
