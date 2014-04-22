/* $Id: formvalue.h 66 2011-11-16 10:54:13Z felfert $
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

#ifndef FORMVALUE_H
#define FORMVALUE_H

#include <string>

#include "ehstypes.h"
#include "contentdisposition.h"

/**
 * This class stores form data sent from the client in GET and POST requests.
 * Metadata within MIME-style attachments is decoded and stored into members of this class.
 * Each element of a form is put into a ContentDisposition object for later retrieval.
 */
class FormValue {

    public:

        /// for MIME attachments only, normal header information like content-type -- everything except content-disposition, which is in oContentDisposition
        StringMap m_oFormHeaders;

        /// everything in the content disposition line
        ContentDisposition m_oContentDisposition; 

        /// the body of the value.  For non-MIME-style attachments, this is the only part used.
        std::string m_sBody; 

        /// Default constructor
        FormValue();

        /**
         * Constructs a new instance.
         * @param irsBody The body content of the form element.
         * @param ioContentDisposition The content disposition type string.
         */
        FormValue(std::string & irsBody, ContentDisposition & ioContentDisposition);

        /// Copy constructor
        FormValue(const FormValue & iroFormValue);

        /// Destructor
        virtual ~FormValue();
};

#endif // FORMVALUE_H
