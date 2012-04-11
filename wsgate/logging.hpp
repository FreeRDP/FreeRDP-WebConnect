/* vim: set et ts=4 sw=4 cindent: */
#ifndef _LOGGING_H_
#define _LOGGING_H_

# include <ostream>
# include <string>
# include <bitset>

namespace wsgate {

    /**
     * A logging class, mapping syslog(3) functionality to C++.
     * For every log level of syslog, a corresponding stream exists.
     */
    class logger {
        public:

            typedef enum {
                AUTH,
                AUTHPRIV,
                CRON,
                DAEMON,
                FTP,
                KERN,
                LOCAL0,
                LOCAL1,
                LOCAL2,
                LOCAL3,
                LOCAL4,
                LOCAL5,
                LOCAL6,
                LOCAL7,
                LPR,
                MAIL,
                NEWS,
                SYSLOG,
                USER,
                UUCP
            } Facility;

            /**
             * Initializes logging.
             * @param ident The program identifier @see openlog
             * @param facility The log facility to use @see openlog
             * @param mask A string, specifying which log levels are enabled.
             *  This string can be specified in multiple formats:
             *  <ul>
             *  <li>A std::bitmask initializer of length 8. The leftmost bit is accociated with the LOG_DEBUG
             *  level, the next bit with LOG_INFO and so on.
             *  <li>A string of log level names (all uppercase). Log level names can be DEBUG, INFO, NOTICE, WARNING, ERR, CRIT, ALERT and EMERG.
             *  </ul>
             */
            logger(const std::string & ident, const Facility facility = DAEMON, const std::string & mask = "11111111");

            /**
             * Copy constructor
             */
            logger(const logger &other);

            ~logger();

            /**
             * Assignment operator
             */
            logger & operator=(const logger &other);

            /**
             * Releases internal log buffers.
             */
            static void release();

            /**
             * Enables logging on existing log channels.
             */
            static void enable();

            /**
             * Disables logging on existing log channels.
             */
            static void disable();

            /**
             * Modifies the set of enabled log levels.
             * @param mask A std::bitset of length 8. The leftmost bit is accociated with the LOG_DEBUG
             *  level, the next bit with LOG_INFO and so on.
             */
            void setmask(std::bitset<8>mask);

            /**
             * Modifies the set of enabled log levels.
             * @param names A string of log level names (all uppercase). Log level names can be DEBUG, INFO, NOTICE, WARNING, ERR, CRIT, ALERT and EMERG.
             */
            void setmaskByName(const std::string & names);

            /**
             * Modifies the log facility.
             * @param facility The log facility to use @see openlog
             */
            void setfacility(Facility facility);

            /**
             * Modifies the log facility.
             * @param facility A string containg the facility name.
             */
            void setfacilityByName(const std::string & facility);

            /**
             * Stream for logging with log level LOG_DEBUG.
             * Writing to this stream before invokation
             * of the constructor does nothing.
             */
            static std::ostream debug;

            /**
             * Stream for logging with log level LOG_INFO.
             * Writing to this stream before invokation
             * of the constructor does nothing.
             */
            static std::ostream info;

            /**
             * Stream for logging with log level LOG_NOTICE.
             * Writing to this stream before invokation
             * of the constructor does nothing.
             */
            static std::ostream notice;

            /**
             * Stream for logging with log level LOG_WARNING.
             * Writing to this stream before invokation
             * of the constructor does nothing.
             */
            static std::ostream warn;

            /**
             * Stream for logging with log level LOG_ERR.
             * Writing to this stream before invokation
             * of the constructor does nothing.
             */
            static std::ostream err;

            /**
             * Stream for logging with log level LOG_CRIT.
             * Writing to this stream before invokation
             * of the constructor does nothing.
             */
            static std::ostream crit;

            /**
             * Stream for logging with log level LOG_ALERT.
             * Writing to this stream before invokation
             * of the constructor does nothing.
             */
            static std::ostream alert;

            /**
            /// Stream for logging with log level LOG_EMERG.
             * Writing to this stream before invokation
             * of the constructor does nothing.
             */
            static std::ostream emerg;

        private:
            /**
             * Converts log level names to a bitset,suitable for invoking setmask(std::bitset<8>mask).
             * @param names A string of log level names (all uppercase). Log level names can be DEBUG, INFO, NOTICE, WARNING, ERR, CRIT, ALERT and EMERG.
             * @return The converted bitset.
             */
            std::bitset<8> namesToBitset(std::string names);

            /**
             * Performs the actual initialization.
             * @param facility The log facility to use @see openlog
             * @param mask A std::bitset of length 8. The leftmost bit is accociated with the LOG_DEBUG
             *  level, the next bit with LOG_INFO and so on.
             */
            void init(Facility facility, std::bitset<8>mask);

            char *ident;

            std::bitset<8> mask;
    };

}

#endif
