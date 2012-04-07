/* vim: set et ts=4 sw=4 cindent: */
#include "logging.h"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <map>

#ifdef HAVE_SYSLOG_H
# include <syslog.h>
#endif
#ifdef HAVE_WINDOWS_H
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

using namespace std;

namespace wsgate {
#ifdef _WIN32
# define LOG_EMERG       0       /* system is unusable */
# define LOG_ALERT       1       /* action must be taken immediately */
# define LOG_CRIT        2       /* critical conditions */
# define LOG_ERR         3       /* error conditions */
# define LOG_WARNING     4       /* warning conditions */
# define LOG_NOTICE      5       /* normal but significant condition */
# define LOG_INFO        6       /* informational */
# define LOG_DEBUG       7       /* debug-level messages */

# define LOG_MASK(pri)   (1 << (pri))            /* mask for one priority */

    static int current_log_mask = 0;
#endif

    class nullbuf : public streambuf
    {
        virtual int overflow(int) {
            return 0;
        }
    };

    ostream logger::debug(new nullbuf);
    ostream logger::info(new nullbuf);
    ostream logger::notice(new nullbuf);
    ostream logger::warn(new nullbuf);
    ostream logger::err(new nullbuf);
    ostream logger::crit(new nullbuf);
    ostream logger::alert(new nullbuf);
    ostream logger::emerg(new nullbuf);

    class syslogbuf : public streambuf
    {
        public:
            syslogbuf(int _level) : level(_level), linebuf("")
        {
        }

        protected:
            virtual int overflow(int c = EOF) {
                if (c == '\n') {
#ifdef _WIN32
                    if (LOG_MASK(level) & current_log_mask) {
                        OutputDebugStringA(linebuf.c_str());
                    }
#else
                    syslog(level, "%s", linebuf.c_str());
#endif
                    linebuf.clear();
                } else
                    linebuf += c;
                return 0;
            }

        private:
            int level;
            string linebuf;
    };

    static const int logmasks[8] = {
        LOG_MASK(LOG_DEBUG),
        LOG_MASK(LOG_INFO),
        LOG_MASK(LOG_NOTICE),
        LOG_MASK(LOG_WARNING),
        LOG_MASK(LOG_ERR),
        LOG_MASK(LOG_CRIT),
        LOG_MASK(LOG_ALERT),
        LOG_MASK(LOG_EMERG)
    };

    void logger::setmaskByName(const string & names)
    {
        setmask(namesToBitset(names));
    }

    bitset<8> logger::namesToBitset(string names)
    {
        static vector<string> masks;
        if (0 == masks.size()) {
            masks.push_back("DEBUG");
            masks.push_back("INFO");
            masks.push_back("NOTICE");
            masks.push_back("WARNING");
            masks.push_back("ERR");
            masks.push_back("CRIT");
            masks.push_back("ALERT");
            masks.push_back("EMERG");
        }
        bitset<8> ret;
        for (size_t i = 0; i < masks.size(); i++) {
            if (names.npos != names.find(masks[i])) {
                ret.set(i);
            }
        }
        return ret;
    }

    void logger::setmask(bitset<8>_mask)
    {
        mask = _mask;
        int bits = 0;
        for (size_t i = 0; i < _mask.size(); i++) {
            if (_mask.test(i)) {
                bits |= logmasks[i];
            }
        }
        // cout << "setlogmask(" << hex << bits << dec << ")" << endl;
#ifdef _WIN32
        current_log_mask = bits;
#else
        setlogmask(bits);
#endif
    }

    void logger::setfacility(const Facility facility)
    {
#ifndef _WIN32
        closelog();
        init(facility, mask);
#endif
    }

    void logger::setfacilityByName(const std::string & facility)
    {
        static map<string, Facility> facmap;
        if (0 == facmap.size()) {
            facmap["AUTH"] = AUTH;
            facmap["AUTPRIV"] = AUTHPRIV;
            facmap["CRON"] = CRON;
            facmap["DAEMON"] = DAEMON;
            facmap["FTP"] = FTP;
            facmap["KERN"] = KERN;
            facmap["LOCAL0"] = LOCAL0;
            facmap["LOCAL1"] = LOCAL1;
            facmap["LOCAL2"] = LOCAL2;
            facmap["LOCAL3"] = LOCAL3;
            facmap["LOCAL4"] = LOCAL4;
            facmap["LOCAL5"] = LOCAL5;
            facmap["LOCAL6"] = LOCAL6;
            facmap["LOCAL7"] = LOCAL7;
            facmap["LPR"] = LPR;
            facmap["MAIL"] = MAIL;
            facmap["NEWS"] = NEWS;
            facmap["SYSLOG"] = SYSLOG;
            facmap["USER"] = USER;
            facmap["UUCP"] = UUCP;
        }
        map<string, Facility>::iterator i = facmap.find(facility);
        if (facmap.end() != i) {
            setfacility(i->second);
        }
    }

    logger::logger(const string & _ident, const Facility facility /*= DAEMON*/, const string & _mask /*= "11111111"*/)
        : ident(strdup(_ident.c_str()))
        , mask()
    {
        string mtmp(_mask);
        if (mtmp.empty()) {
            mtmp = "11111111";
        }
        if (mtmp.npos != mtmp.find_first_not_of("01")) {
            init(facility, namesToBitset(mtmp));
        } else {
            bitset<8> bits(mtmp);
            init(facility, bits);
        }
    }

    logger::logger(const logger &other)
        : ident(strdup(other.ident))
        , mask(other.mask)
    {
    }

    logger & logger::operator=(const logger &other)
    {
        if (this != &other) {
            if (NULL != ident) {
                free(ident);
            }
            ident = strdup(other.ident);
        }
        return *this;
    }

    logger::~logger()
    {
#ifndef _WIN32
        closelog();
#endif
        release();
        free(ident);
    }

    void logger::release()
    {
        delete debug.rdbuf();
        delete info.rdbuf();
        delete notice.rdbuf();
        delete warn.rdbuf();
        delete err.rdbuf();
        delete crit.rdbuf();
        delete alert.rdbuf();
        delete emerg.rdbuf();
    }

    void logger::enable()
    {
        delete debug.rdbuf(new syslogbuf(LOG_DEBUG));
        delete info.rdbuf(new syslogbuf(LOG_INFO));
        delete notice.rdbuf(new syslogbuf(LOG_NOTICE));
        delete warn.rdbuf(new syslogbuf(LOG_WARNING));
        delete err.rdbuf(new syslogbuf(LOG_ERR));
        delete crit.rdbuf(new syslogbuf(LOG_CRIT));
        delete alert.rdbuf(new syslogbuf(LOG_ALERT));
        delete emerg.rdbuf(new syslogbuf(LOG_EMERG));
    }

    void logger::disable()
    {
        delete debug.rdbuf(new nullbuf);
        delete info.rdbuf(new nullbuf);
        delete notice.rdbuf(new nullbuf);
        delete warn.rdbuf(new nullbuf);
        delete err.rdbuf(new nullbuf);
        delete crit.rdbuf(new nullbuf);
        delete alert.rdbuf(new nullbuf);
        delete emerg.rdbuf(new nullbuf);
    }

    void logger::init(Facility facility, bitset<8>mask)
    {
#ifndef _WIN32
        int fac = LOG_DAEMON;
        switch (facility) {
            case AUTH:
                fac = LOG_AUTH;
                break;
            case AUTHPRIV:
                fac = LOG_AUTHPRIV;
                break;
            case CRON:
                fac = LOG_CRON;
                break;
            case DAEMON:
            default:
                fac = LOG_DAEMON;
                break;
            case FTP:
                fac = LOG_FTP;
                break;
            case KERN:
                fac = LOG_KERN;
                break;
            case LOCAL0:
                fac = LOG_LOCAL0;
                break;
            case LOCAL1:
                fac = LOG_LOCAL1;
                break;
            case LOCAL2:
                fac = LOG_LOCAL2;
                break;
            case LOCAL3:
                fac = LOG_LOCAL3;
                break;
            case LOCAL4:
                fac = LOG_LOCAL4;
                break;
            case LOCAL5:
                fac = LOG_LOCAL5;
                break;
            case LOCAL6:
                fac = LOG_LOCAL6;
                break;
            case LOCAL7:
                fac = LOG_LOCAL7;
                break;
            case LPR:
                fac = LOG_LPR;
                break;
            case MAIL:
                fac = LOG_MAIL;
                break;
            case NEWS:
                fac = LOG_NEWS;
                break;
            case SYSLOG:
                fac = LOG_SYSLOG;
                break;
            case USER:
                fac = LOG_USER;
                break;
            case UUCP:
                fac = LOG_UUCP;
                break;
        }
        openlog(ident, LOG_PID|LOG_NDELAY, fac);
        // cout << "openlog('" << ident << "', LOG_PID|LOG_NDELAY," << hex << fac << dec << ")" << endl;
#endif
        setmask(mask);
        enable();
    }

}
