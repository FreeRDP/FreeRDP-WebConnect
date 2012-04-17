/* $Id: common.h 93 2012-03-31 13:47:58Z felfert $
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
