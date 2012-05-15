/* vim: set et ts=4 sw=4 cindent:
 *
 * FreeRDP-WebConnect,
 * A gateway for seamless access to your RDP-Sessions in any HTML5-compliant browser.
 *
 * Copyright 2012 Fritz Elfert <wsgate@fritz-elfert.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

/**
 * The namespace of the main proxy application.
 */
namespace wsgate {

/**
 * A small helper class for providing cross-platform non-blocking keyboard input.
 */
class kbdio {
    public:
        /// Constructor
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
        /// Destructor
        ~kbdio() {
            tcsetattr(0, TCSANOW, &stsave);
        }

        /**
         * Check if the user pressed 'q'.
         * @return true, if the 'q' key was pressed.
         */
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

}

#endif
