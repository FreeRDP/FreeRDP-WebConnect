/* $Id$
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

#ifndef _COMMON_H_
#define _COMMON_H_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_CONIO_H
# include <conio.h>
#endif
#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string>

#ifdef _WIN32
inline void sleep(int seconds) { Sleep(seconds * 1000); }
#endif

// A small helper class for providing
// non-blocking keyboard input.
class kbdio {
    public:
        kbdio()
#ifndef _WIN32
            : st(termios()), stsave(termios())
#endif
        {
#ifndef _WIN32
            tcgetattr(0, &stsave);
            memcpy(&st, &stsave, sizeof(st));
            st.c_lflag &= ~ICANON;
            st.c_cc[VMIN] = st.c_cc[VTIME] = 0;
            tcsetattr(0, TCSANOW, &st);
#endif
        }
#ifdef _WIN32
        bool qpressed() {
            char c;
            bool ret = false;
            while (0 != _kbhit()) {
                c = _getch();
                ret |= ('q' == c);
            }
            return ret;
        }
#else
        ~kbdio() {
            tcsetattr(0, TCSANOW, &stsave);
        }
        bool qpressed() {
            char c;
            bool ret = false;
            while (1 == read(0, &c, 1)) {
                ret |= ('q' == c);
            }
            return ret;
        }
    private:
        struct termios st;
        struct termios stsave;
#endif
};

std::string basename(const std::string & s)
{
    std::string ret(s);
    size_t pos = ret.rfind("/");
    if (pos != ret.npos)
        ret.erase(0, pos);
#ifdef _WIN32
    pos = ret.rfind("\\");
    if (pos != ret.npos)
        ret.erase(0, pos);
#endif
    return ret;
}

#endif
