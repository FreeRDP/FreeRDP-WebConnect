/* $Id: formvalue.cpp 30 2010-06-05 19:08:00Z felfert $
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

#include <string>

#include "formvalue.h"
#include "contentdisposition.h"

FormValue::FormValue() :
    m_oFormHeaders(StringMap()),
    m_oContentDisposition(ContentDisposition()),
    m_sBody("")
{
}

FormValue::FormValue(std::string & irsBody,
        ContentDisposition & ioContentDisposition) :
    m_oFormHeaders(StringMap()),
    m_oContentDisposition(ioContentDisposition),
    m_sBody(irsBody)
{
}

FormValue::FormValue(const FormValue & other) :
    m_oFormHeaders(other.m_oFormHeaders),
    m_oContentDisposition(other.m_oContentDisposition),
    m_sBody(other.m_sBody)
{
}

FormValue::~FormValue()
{
}
